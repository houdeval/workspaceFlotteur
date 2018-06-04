/*  PIC18F14K22  mikroC PRO for PIC v6.4
Oscillateur interne 16MHZ (attention l'edit project ne marche pas, param�trer 
l'oscillateur en dur)


Hardware:
  18F14K22  DIP20,SOIC
  
  pin 1     VDD Alim +5V
  pin 2     OSC1/RA5 ---> sortie commande de l'alimentation g�n�rale
  pin 3     OSC2/RA4 ---> sortie commande LED
  pin 4     RA3/MCR
  pin 5     RC5
  pin 6     RC4
  pin 7     RC3 ---> mesure VBAT4
  pin 8     RC6
  pin 9     RC7
  pin 10    RB7
  pin 11    RB6 ---> SCL I2C clock input
  pin 12    RB5
  pin 13    RB4 ---> SDA I2C data output
  pin 14    RC2 ---> mesure VBAT3
  pin 15    RC1 ---> mesure VBAT2
  pin 16    RC0 ---> mesure VBAT1
  pin 17    RA2/INT2/T0CKI ---> entr�e ILS
  pin 18    RA1/INT1
  pin 19    RA0/INT0 ---> sortie LED
  pin 20    VSS Alim 0V


Programme pour commander l'alimentation g�n�rale d'un syst�me (via un MOSFET de
puissance IPB80P03P4L) � partir d'un ILS connect� sur l'entr�e INT2, le programme
contr�le la tension d'alimentation de 4 batteries, la valeur est retourn�e sur 
ordre via l'I2C.
La sortie RA4 commande trois LED de rep�rage via le circuit ZXLD1350.

14/03/18/ Implantation et essai du programme, seuil des batteries � corriger

*/

sbit BAT1 at PORTC.B0; // entr�e de controle de tension BAT1
sbit BAT2 at PORTC.B1; // entr�e de controle de tension BAT2
sbit BAT3 at PORTC.B2; // entr�e de controle de tension BAT2
sbit BAT4 at PORTC.B3; // entr�e de controle de tension BAT2
sbit ILS at PORTA.B2; //  entr�e de l'ILS

sbit LED1 at LATA.B1; // sortie LED1
sbit LED at LATA.B0; // sortie LED
sbit LED_PUISSANCE at LATA.B4; // sortie LED de puissance
sbit ALIM at LATA.B5; // sortie MOSFET de puissance, commande de l'alimentation

// I2C
#define ADDRESS_I2C 0x72;
#define SIZE_RX_BUFFER 3
unsigned short rxbuffer_tab[SIZE_RX_BUFFER];
unsigned short tmp_rx = 0;
unsigned short nb_tx_octet = 0;
unsigned short nb_rx_octet = 0;

// ILS
#define ILS_CPT_TIME 4
unsigned short ils_cpt = 4;
unsigned short ils_removed = 1;

// State Machine
enum power_state {IDLE,POWER_ON,WAIT_TO_SLEEP, SLEEP};
unsigned short state = IDLE;

// Batteries
#define WARNING_LOW_VOLTAGE 665 // 0.015625 (quantum) / 10.4 (min tension)
unsigned int battery_voltage[4];
unsigned short battery_global_default = 0;

// Flasher LED
unsigned short start_led_puissance = 0;
unsigned short led_puissance_delay = 20; // 100ms * val (default = 2s)
unsigned short cpt_led_puissance = 20;

// Led
unsigned short led_delay = 100;
unsigned short cpt_led = 100;
unsigned short set_led_on = 0;

// Sleep mode
unsigned char time_to_start[3] = {1, 0, 0}; // hour, min, sec
unsigned char time_to_stop = 60; // in sec (max 255 sec)

unsigned char default_time_to_start[3] = {1, 0, 0};
unsigned char default_time_to_stop = 60;

unsigned short start_time_to_stop = 0;
unsigned short start_time_to_power_on = 0;
unsigned short start_time_to_start = 0;

unsigned short k = 0;

/**
 * @brief i2c_read_data_from_buffer
 */
void i2c_read_data_from_buffer(){
  switch(rxbuffer_tab[0]){
  case 0x00:  // alimentation
    switch(rxbuffer_tab[1]){
    case 0x00:
      state = IDLE;
      break;
    case 0x01:
      state = POWER_ON;
      break;
    case 0x02:
      time_to_stop = default_time_to_stop;  // Go to Sleep mode
      start_time_to_stop = 1;
      state = WAIT_TO_SLEEP;
      break;
    default:
      break;
    }
    break;
  case 0x01:  // led power
    if(rxbuffer_tab[1] == 0)
      start_led_puissance = 0;
    else
      start_led_puissance = 1;
    LED_PUISSANCE = 0;
    break;
  case 0x02:
    led_puissance_delay = rxbuffer_tab[1];
    break;
  case 0x03:
    default_time_to_start[0] = rxbuffer_tab[1]; // hours
    break;
  case 0x04:
    default_time_to_start[1] = rxbuffer_tab[1]; // min
    break;
  case 0x05:
    default_time_to_start[2] = rxbuffer_tab[1]; // sec
    break;
  case 0x06:
    default_time_to_stop = rxbuffer_tab[1]; // sec
    break;
  default:
    break;
  }
}

/**
 * @brief i2c_write_data_to_buffer
 * @param nb_tx_octet
 */
void i2c_write_data_to_buffer(unsigned short nb_tx_octet){
  switch(rxbuffer_tab[0]+nb_tx_octet){
  case 0x00:
    SSPBUF = battery_voltage[0];
    break;
  case 0x01:
    SSPBUF = battery_voltage[0] >> 8;
    break;
  case 0x02:
    SSPBUF = battery_voltage[1];
    break;
  case 0x03:
    SSPBUF = battery_voltage[1] >> 8;
    break;
  case 0x04:
    SSPBUF = battery_voltage[2];
    break;
  case 0x05:
    SSPBUF = battery_voltage[2] >> 8;
    break;
  case 0x06:
    SSPBUF = battery_voltage[3];
    break;
  case 0x07:
    SSPBUF = battery_voltage[3] >> 8;
    break;
  case 0x08:
    SSPBUF = start_led_puissance;
    break;
  case 0x09:
    SSPBUF = state;
    break;
  default:
    SSPBUF = 0x00;
    break;
  }
}

/**
 * @brief read batteries voltage
 */
void read_batteries_voltage(){
  battery_voltage[0] = ADC_Get_Sample(4);   // Get 10-bit results of AD conversion AN4 batterie 1
  battery_voltage[1] = ADC_Get_Sample(5);   // Get 10-bit results of AD conversion AN5 batterie 2
  battery_voltage[2] = ADC_Get_Sample(6);   // Get 10-bit results of AD conversion AN6 batterie 3
  battery_voltage[3] = ADC_Get_Sample(7);   // Get 10-bit results of AD conversion AN7 batterie 4
}

/**
 * @brief analyze batteries voltage
 * 3 cellules lithium --> 4.1v/�l�ments --> en fin de charge
 * 9v totalement d�charg�
 */
void analyze_batteries_voltage(){
     unsigned short l = 0;
  battery_global_default = 0;

  for(l=0; l<4; l++){
    if(battery_voltage[l] < WARNING_LOW_VOLTAGE)
      battery_global_default = 1;
  }
}

/**
 * @brief initialisation de l'I2C en mode esclave
 */
void init_i2c(){
  SSPADD = ADDRESS_I2C; // Address Register, Get address (7bit). Lsb is read/write flag
  SSPCON1 = 0x3E; // SYNC SERIAL PORT CONTROL REGISTER
  SSPCON1.SSPEN = 1;
  // bit 3-0 SSPM3:SSPM0: I2C Firmware Controlled Master mode,
  // 7-bit address with START and STOP bit interrupts enabled
  // bit 4 CKP: 1 = Enable clock
  // bit 5 SSPEN: Enables the serial port and configures the SDA and SCL
  // pins as the source of the serial port pins

  SSPCON2 = 0x00;
  SSPSTAT=0x00;
  SSPSTAT.SMP = 1; // 1 = Slew rate control disabled for standard speed mode (100 kHz and 1 MHz)
  SSPSTAT.CKE = 1; // 1 = Input levels conform to SMBus spec

  PIE1.SSPIE = 1; // Synchronous Serial Port Interrupt Enable bit
  PIR1.SSPIF = 0; // Synchronous Serial Port (SSP) Interrupt Flag, I2C Slave
  // a transmission/reception has taken place.
  PIR2.BCLIF = 0;
}

/**
 * @brief InitINT2
 * Fonction d'initialisation de l'entr�e d'interruption INT2 pour la d�tection de fermeture/ouverture
 * de l'ILS, d�tection du front montant (ouverture de l'ILS).
 */
// void init_io_ils(){
//     INTCON2.INTEDG2 = 0; // Interrupt on falling edge
//     INTCON3.INT2IE = 0; //Enables the INT2 external interrupt
//     INTCON3.INT2IF = 0; // INT2 External Interrupt Flag bit
// }

/**
 * @brief init_timer0
 * Fonction d'initialisation du TIMER0
 * Prescaler 1:128; TMR0 Preload = 3036; Actual Interrupt Time : 1 s
 */
void init_timer0(){
  //T0CON = 0x86; // TIMER0 ON time 2s
  T0CON = 0x85; // TIMER0 ON (1 s)
  TMR0H = 0x0B;
  TMR0L = 0xDC;
  TMR0IE_bit = 0;
}

/**
 * @brief init_timer1
 * Fonction d'initialisation du TIMER1
 * Prescaler 1:8; TMR1 Preload = 15536; Actual Interrupt Time : 100 ms
 */
// void init_timer1(){
//     T1CON = 0x31;
//     TMR1IF_bit = 0;
//     TMR1H = 0x77;
//     TMR1L = 0x48;
//     TMR1IE_bit = 0;
// }

/**
 * @brief init_timer2
 * Fonction d'initialisation du TIMER2
 * Prescaler 1:1; Postscaler 1:2; TMR2 Preload = 199; Actual Interrupt Time : 10 us
 */
// void init_timer2(){
//     T2CON = 0x08;
//     TMR2IE_bit = 0;
//     PR2 = 79;
//     INTCON = 0xC0;
// }

/**
 * @brief init_timer3
 * Fonction d'initialisation du TIMER3
 * Prescaler 1:8; TMR1 Preload = 15536; Actual Interrupt Time : 100 ms
 */
void init_timer3(){
  T3CON = 0x30;
  TMR3IF_bit = 0;
  TMR3H = 0x3C;
  TMR3L = 0xB0;
  TMR3IE_bit = 0;
}

/**
 * @brief init_io
 * Initialisation des entr�es sorties du PIC
 */
void init_io(){
  ANSEL = 0xF0;  // Set RC0,RC1,RC2,RC3 to analog (AN4,AN5,AN6,AN7)

  CM1CON0 = 0x00; // Not using the comparators
  CM2CON0 = 0x00;

  // NVCFG = 00; PVCFG = 00;

  TRISA = 0xFF;
  //TRISA1_bit = 1; // RA1 en entr�e
  TRISA0_bit = 0; // RA0 en sortie
  TRISA2_bit = 1; // RA2 en entr�e
  TRISA4_bit = 0; // RA4 en sortie
  TRISA5_bit = 0; // RA5 en sortie

  TRISA1_bit = 0; // RA1 en sortie

  INTCON2.RABPU = 0; // PORTA and PORTB Pull-up Enable bit
  //WPUA.WPUA1 = 1; // Pull-up enabled sur RA2, sur inter de but�e basse
  WPUA.WPUA2 = 1; // Pull-up enabled sur RA2, sur inter de but�e basse
  
  TRISB4_bit = 1; // RB4 en entr�e
  TRISB6_bit = 1; // RB6 en entr�e

  TRISB5_bit = 1; // RB5 en entr�e
  TRISB7_bit = 0; // RB6 en sortie

  TRISC = 0xFF;
  TRISC0_bit = 1; // RC0 en entree voie AN4
  TRISC1_bit = 1; // RC1 en entree voie AN5
  TRISC2_bit = 1; // RC2 en entree voie AN6
  TRISC3_bit = 1; // RC3 en entree voie AN7
}


/**
 * @brief main
 */
void main(){
  // Oscillateur interne de 16Mhz
  OSCCON = 0b01110010;   // 0=4xPLL OFF, 111=IRCF<2:0>=16Mhz  OSTS=0  SCS<1:0>10 1x = Internal oscillator block

  init_io(); // Initialisation des I/O
  init_i2c(); // Initialisation de l'I2C en esclave
  init_timer0(); // Initialisation du TIMER0 toutes les 2 secondes
  init_timer3(); // Initialisation du TIMER3 toutes les 100ms

  ADC_Init();

  LED = 0; // sortie LED
  LED1 = 0;
  LED_PUISSANCE = 0; // sortie LED de puissance
  ALIM = 0; // sortie MOSFET de puissance, commande de l'alimentation
  battery_global_default = 0;

  //UART1_Init(9600);

  INTCON3.INT1IP = 1; //INT1 External Interrupt Priority bit, INT0 always a high
  //priority interrupt source

  IPR1.SSPIP = 0; //Master Synchronous Serial Port Interrupt Priority bit, low priority
  RCON.IPEN = 1;  //Enable priority levels on interrupts
  INTCON.GIEH = 1; //enable all high-priority interrupts
  INTCON.GIEL = 1; //enable all low-priority interrupts

  INTCON.GIE = 1; // Global Interrupt Enable bit
  INTCON.PEIE = 1; // Peripheral Interrupt Enable bit

  TMR0IE_bit = 1;  //Enable TIMER0
  TMR3IE_bit = 1;  //Enable TIMER3
  TMR3ON_bit = 1; // Start TIMER3
  TMR0ON_bit = 1; // Start TIMER1

  while(1){
    read_batteries_voltage();
    analyze_batteries_voltage();
    
    //UART1_Write(state);
    
    switch (state){
    case IDLE: // Idle state
      ALIM = 0;
      led_delay = 50;

      if(ILS==0){ // Magnet detected
        ils_cpt--;
        set_led_on = 1;
      }
      else{
        ils_cpt = ILS_CPT_TIME;
        set_led_on = 0;
        ils_removed = 1;
      }

      if(ils_removed == 1 && ils_cpt == 0){
        ils_cpt = ILS_CPT_TIME;
        state = POWER_ON;
        ils_removed = 0;
        set_led_on = 0;
      }
      break;

    case POWER_ON:
      ALIM = 1;
      if(battery_global_default == 1)
        led_delay = 5; // 0.5 sec
      else
        led_delay = 20; // 5 sec

      if(ILS==0){ // Magnet detected
        ils_cpt--;
        set_led_on = 1;
      }
      else{
        ils_cpt = ILS_CPT_TIME;
        set_led_on = 0;
        ils_removed = 1;
      }

      if(ils_removed == 1 && ils_cpt == 0){
        ils_cpt = ILS_CPT_TIME;
        state = IDLE;
        ils_removed = 0;
        set_led_on = 0;
      }

      break;

    case WAIT_TO_SLEEP:

      ALIM = 1;
      led_delay = 1;
      start_time_to_stop = 1;
      if(time_to_stop==0){
        start_time_to_stop = 0;
        for(k=0; k<3; k++)
          time_to_start[k] = default_time_to_start[k];
        start_time_to_start = 1;
        state = SLEEP;
      }
      break;

    case SLEEP:
      ALIM = 0;
      led_delay = 600;
      if(time_to_start[0] == 0 && time_to_start[1] == 0 && time_to_start[2] == 0){
        state = POWER_ON;
        start_time_to_start = 0;
      }
      break;
    default:
      state = POWER_ON;
      break;
    }
    delay_ms(500);
  }
}


/**
 * @brief interrupt_low
 */
void interrupt_low(){
    // Interruption sur le bus I2C, le bus est en esclave
    // Interruption sur Start & Stop

    if (PIR1.SSPIF){  // I2C Interrupt
    
        if (SSPSTAT.R_W == 1){   //******  transmit data to master ****** //
            i2c_write_data_to_buffer(nb_tx_octet);
            nb_tx_octet++;
            delay_us(10);
            SSPCON1.CKP = 1;
        }
        else{ //****** recieve data from master ****** //
            if (SSPSTAT.BF == 1){ // Buffer is Full (transmit in progress)
                if (SSPSTAT.D_A == 1){ //1 = Indicates that the last byte received or transmitted was data  
                    if(nb_rx_octet < SIZE_RX_BUFFER)
                        rxbuffer_tab[nb_rx_octet] = SSPBUF;
                    nb_rx_octet++;
                }
                else{
                     nb_tx_octet = 0;
                     nb_rx_octet = 0;
                }
            }
            else{ // At the end of the communication
                i2c_read_data_from_buffer();
            }
            tmp_rx = SSPBUF;
        }

        PIR1.SSPIF = 0; // reset SSP interrupt flag
    }
    
    if (PIR2.BCLIF)
        PIR2.BCLIF = 0;
}

/**
 * @brief interrupt
 * Fonction de gestion des interruptions:
 * interruption sur TIMER3 interruptions tous les 100ms pour visu STATE0(tous les 500ms)
 * interruption sur le bus I2C
 */
void interrupt(){
  /// ************************************************** //
  /// ********************** TIMERS  ******************* //

  // Interruption du TIMER0 (1 s) (cpt to start/stop)
  if (TMR0IF_bit){

    // To Do
    if(start_time_to_start == 1){
      if(time_to_start[2]>0){
        time_to_start[2]--;
      }
      else{
        if(time_to_start[1]>0){
          time_to_start[1]--;
          time_to_start[2]=59;
        }
        else{
          if(time_to_start[0]>0){
            time_to_start[0]--;
            time_to_start[1]=59;
          }
        }
      }
    }

    if(start_time_to_stop == 1){
      if(time_to_stop>0)
        time_to_stop--;
    }

    TMR0H = 0x0B;
    TMR0L = 0xDC;
    TMR0IF_bit = 0;
  }

  // Interruption du TIMER3 (Led Puissance)
  if (TMR3IF_bit){
    // LED Puissance
    if(start_led_puissance == 1){
      if (cpt_led_puissance > 0){
        LED_PUISSANCE = 0;
        cpt_led_puissance--;
      }
      else{
        LED_PUISSANCE = 1;
        cpt_led_puissance=led_puissance_delay;
      }
    }
    else{
      LED_PUISSANCE = 0;
    }

    // LED
    if(set_led_on == 1) // For ILS
      LED = 1;
    else{
      if (cpt_led > 0){
        LED = 0;
        cpt_led--;
      }
      else{
        LED = 1;
        cpt_led=led_delay;
      }
    }

    TMR3H = 0x3C;
    TMR3L = 0xB0;
    TMR3IF_bit = 0;
  }
}