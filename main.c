/* 
 * File:   main.c
 * Author: lgd
 *
 * Created on 19 mai 2017, 14:13
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mcc_generated_files/mcc.h"
#include "mcc_generated_files/interrupt_manager.h"

static uint16_t adcResult;

int toggle_leds(void) {
    D4_Toggle();
    D5_Toggle();
    D6_Toggle();
    D7_Toggle();
    return (EXIT_SUCCESS);
}

int set_high_leds(void) {
    D4_SetHigh();
    D5_SetHigh();
    D6_SetHigh();
    D7_SetHigh();
    return (EXIT_SUCCESS);
}

int set_low_leds(void) {
    D4_SetLow();
    D5_SetLow();
    D6_SetLow();
    D7_SetLow();
    return (EXIT_SUCCESS);
}

/*
 * Fait clignoter `n` fois les quatres premiers bits
 * (de poids faible) de l?octet `c` sur les diodes.
 */
int blink_leds(int n, int c) {
    __delay_ms(640);
    for (int i = 0; i < n; ++i) {
        set_low_leds();
        __delay_ms(128);
        if (c & 0b00000001) D4_SetHigh();
        if (c & 0b00000010) D5_SetHigh();
        if (c & 0b00000100) D6_SetHigh();
        if (c & 0b00001000) D7_SetHigh();
        __delay_ms(128);
    }
    set_low_leds();
    __delay_ms(640);
    return (EXIT_SUCCESS);
}

/*
 * Fonction de déboguage d?affichage d?un octet
 * sur les quatres diodes, en deux fois.
 * 
 * Appuyer sur le bouton "S1" après la vision
 * des quatres premiers bits puis des quatres suivants
 * pour passer à la suite du code.
 */
int show_char_leds(char c) {
    // clignotement préliminaire, double
    set_low_leds();
    __delay_ms(64);
    set_high_leds();
    __delay_ms(64);
    set_low_leds();
    __delay_ms(64);
    set_high_leds();
    __delay_ms(64);
    set_low_leds();
    
    // affichage des bits de poids faible
    if (c & 0b00000001) D4_SetHigh();
    if (c & 0b00000010) D5_SetHigh();
    if (c & 0b00000100) D6_SetHigh();
    if (c & 0b00001000) D7_SetHigh();
    
    // attente de l?appui sur le bouton
    while (S1_GetValue());
    
    // clignotement intermédiaire, simple
    __delay_ms(640);
    set_low_leds();
    __delay_ms(64);
    set_high_leds();
    __delay_ms(64);
    set_low_leds();
    
    // affichage des bits de poids fort
    if (c & 0b00010000) D4_SetHigh();
    if (c & 0b00100000) D5_SetHigh();
    if (c & 0b01000000) D6_SetHigh();
    if (c & 0b10000000) D7_SetHigh();
    
    // attente de l?appui sur le bouton
    while(S1_GetValue());
    __delay_ms(640);
    // remise à zéro
    set_low_leds();
    
    return (EXIT_SUCCESS);
}

/*
 * Affiche pendant tout le reste du programme
 * les octets reçus par l?UART sur les diodes.
 */
int show_input_leds(void) {
    while(true) {
        char c = 0;
        while (c == 0) {
            c = EUSART_Read();
        }
        show_char_leds(c);
    }
    return (EXIT_SUCCESS);
}

/*
 * Lecture d?un ligne (maximum 62 caractères) reçue sur l?UART.
 * La fonction attend et lit "\r\n" à la fin de la ligne
 * et remplace cet "\r\n" par "\0" (fin de chaîne.)
 */
char * read_line(void) {
    char c[64];
    // On passe les premiers octets nuls ('\0')
    do {
        c[0] = EUSART_Read();
    } while (c[0] == '\0');
    // Lecture d?une ligne complète (terminée par "\r\n")
    int i = 0;
    do {
        c[++i] = EUSART_Read();
    } while (c[i] != '\n' && i < 64);
    // On remplace "\r\n" par "\0\0"
    if (c[i] == '\n') c[i] = '\0';
    if (c[i-1] == '\r') c[i-1] = '\0';
    return c;
}

/*
 * Envoie d?une ligne sur l?interface UART.
 * La fonction rajoute "\r\n" à la fin.
 */
int write_line(char * line) {
    int i = 0;
    while(line[i] != '\0') {
        EUSART_Write(line[i++]);
    }
    EUSART_Write('\r');
    EUSART_Write('\n');
    return (EXIT_SUCCESS);
}

/*
 * Envoi de la ligne `cmd` et comparaison de la réponse
 * avec la chaîne de caractère `res`.
 */
int write_and_wait(char * cmd, char * res) {
    write_line(cmd);
    char * line = read_line();
    return strcmp(line, res);
}



void main(void) {

    // initialize the device
    SYSTEM_Initialize();
    INTERRUPT_GlobalInterruptEnable();					
	INTERRUPT_PeripheralInterruptEnable();

    SWAKE_SetHigh();

    int err = 0;
    
    // Attente de "CMD"
    err = strcmp(read_line(), "CMD");
    if (err) blink_leds(1, 1);
    
    // Remise aux valeurs d?usine
    err = write_and_wait("SF,1", "AOK");
    if (err) blink_leds(1, 2);
    
    // Prise en charge des services "Device Information" et "Battery" et
    // le service privé
    err = write_and_wait("SS,C0000001", "AOK");
    if (err) blink_leds(1, 4);
    
    // Fait fonctionner le module en mode "périphérique"
    err = write_and_wait("SR,00000000", "AOK");
    if (err) blink_leds(2, 1);
    
    // Renommage du module
    err = write_and_wait("S-,PIC16-BLE2-001", "AOK");
    if (err) blink_leds(2, 2);
    
    // Supprime l?ancien service privé
    err = write_and_wait("PZ", "AOK");
    if (err) blink_leds(2, 6);
    
    // UUID du service privé
    err = write_and_wait("PS,11223344556677889900AABBCCDDEEFF", "AOK");
    if (err) blink_leds(2, 7);
    
    // UUID de la carac. privée, lisible et notification (0x12) et de deux octets (0x02)
    err = write_and_wait("PC,010203040506070809000A0B0C0D0E0F,12,02", "AOK");
    if (err) blink_leds(2, 8);

    // UUID de la carac. privée, lisible, éditable et notifiable (0x1A) et de trois octets (0x03)
    err = write_and_wait("PC,FF0203040506070809000A0B0C0D0E0F,1A,03", "AOK");
    if (err) blink_leds(2, 9);
            
    // Redémarre le module
    err = write_and_wait("R,1", "Reboot");
    if (err) blink_leds(2, 3);
    
    // Attente de "CMD"
    err = strcmp(read_line(), "CMD");
    if (err) blink_leds(2, 4);
    
    // Met le niveau de la batterie à 50%
    err = write_and_wait("SUW,2A19,32", "AOK");
    if (err) blink_leds(2, 5);
    
    // Met la carac. privée à une valeur donnée
    err = write_and_wait("SUW,010203040506070809000A0B0C0D0E0F,AABBCCDDEE", "AOK");
    if (err) blink_leds(2, 10);

    // Publie les services BLE
    //err = write_and_wait("A", "AOK");
    //if (err) blink_leds(2, 14);
    
    /*
    // envoi périodique d?un niveau de batterie
    char * cmd = "SUW,2A19,30";
    // boucle infinie
    while(true) {
        __delay_ms(1000);
        err = write_line(cmd);
        if (err) blink_leds(1, 15);
    }
    */
    int c = 0;
    while(true) {
        // advertisement interval=80ms period=2s
        c++;
        if (c%5==0) {
            err = write_line("A,0050,07D0");
            if (err) blink_leds(2, 14);
        }
        if(c>100) {
            c=0;
        }
        __delay_ms(1000);
        err = write_line("A,0050,07D0");
        if (err) blink_leds(2, 14);
        adcResult = ADC_GetConversion(0x4);
        char result[4];
        char command[64]="SUW,010203040506070809000A0B0C0D0E0F,";
        // Met le niveau de la batterie à 50%
        err = write_line("SUW,2A19,32");
        if (err) blink_leds(2, 5);
        sprintf(result, "%X", adcResult);
        err=write_line(strcat(command,result));
    }
    
    // Arrête la connexion UART
    SWAKE_SetLow();
    
    // Vérifie le bon arrêt de la connexion UART
    err = strcmp(read_line(), "END");
    if (err) blink_leds(3,9);
    
    // boucle infinie
    while(true);
}