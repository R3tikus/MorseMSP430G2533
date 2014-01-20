/*
|===============================================================================
|PROGRAMA: MORSE FOR MSP430G2533
|VERSION: 1
|DESCRIPCION: PEQUEÑO DECODIFICADOR DE MORSE CON EL LAUNCHPAD MSP430G2533.
|             PULSANDO EL PULSADOR S2 MENOS DE 0.5 SEG GENERAMOS UN PUNTO Y MÁS DE
|             0.5 SEGUNDOS UNA RAYA.
|             PARA DECODFICAR LA SECUENCIA GENERAREMOS UNA PAUSA DE MAS DE UN SEGUNDO.
|RECURSOS: LEDS(LED1 Y LED2), PULSADOR(S2), WATCHDOG, TIMERS, USCIA0
|FECHA: 24/06/2013
|===============================================================================
*/

//INCLUDES
#include "io430g2553.h"

//CONSTANT
#define ON 1  //Activado
#define OFF 0 //Desactivado
#define LED_ON 1 //Led Encendido
#define LED_OFF 0 //Led Apagado
#define BUTTON_ON 0 //Boton Presionado
#define BUTTON_OFF 1 //Boton Soltado
#define BUFFER 50 
#define DOT 0 //Punto
#define DASH 1 //Raya

typedef union
{
  
        unsigned char total;

        struct
        {
          unsigned char f0:1; //Pulsacion S2
          unsigned char f1:1; //Liberacion S2
          unsigned char f2:1; //Enciende LED1
          unsigned char f3:1; //Enciende LED2
          unsigned char f4:1;
          unsigned char f5:1;
          unsigned char f6:1;
          unsigned char f7:1;
        };
}flags;

//VARIABLES GLOBALES
flags task;

unsigned char symbol; //simbolo morse
unsigned char num_interrupt; //numero de interrupciones del Timer1
unsigned char pause; //Para saber en la pausa que estamos
unsigned char last_pause; //Ultima pausa
unsigned char c_sequence; //Contador para incrementar las posiciones en la secuencia
unsigned char c_message; //Contador para incrementar en el mensaje
unsigned char sequence[BUFFER]; //Secuencia de Simbolos
unsigned char message[BUFFER]; //Mensaje

//FUCIONES A USAR
unsigned char Decodificar(unsigned char *secuencia); //Decodifica la secuencia
void Config_uC(void); //Configura el uC
void Config_Peripherals(void); //Configura los perifericos
void init_variables(void); //Inicializa las variables

/************************************************************************/
//MAIN DEL PROGRAMA 
int main(void)
{
  //Config uC
  Config_uC();
  
  //Config Peripherals
  Config_Peripherals();
  
  //Inicializa las Variables
  init_variables();
  
  //Habilita las interrupciones
  __enable_interrupt();
  
  for(;;)
  {
    //Activa el modo bajo consumo
    __low_power_mode_0();
    
    //Si hay alguna tarea activa 
    while(task.total != OFF)
    {
      /********************************/
      
      //si task.f0 activa
      if(task.f0 == ON)
      {
        task.f0 = OFF; //Limpia la flag
        //si hay pausa pon el contador de secuencia a 0
        //comienza una nueva secuencia
        if(last_pause != 0)
        {
          c_sequence = 0;
          
        }
        
      }//end if(task.f0 == ON)
      
      /********************************/
      //si task.f1 activada
      //Si hemos soltado el boton activa esta tarea
      //Añade el simbolo correspondiente a la secuencia
      if(task.f1 == ON)
      {
        task.f1 = OFF; //Limpia el flag
        
        if(symbol == DASH)
        {
          sequence[c_sequence] = '-';
        }
        else
        {
          sequence[c_sequence] = '.';
        }
        
        c_sequence++; //+1 para el siguiente simbolo
        
      }//end if(task.f1 == ON)
      
      /********************************/
      //LED1 ON
      if(task.f2 == ON)
      {
        task.f2 = OFF; //limpia la flag
        P1OUT_bit.P0 = LED_ON; //led1 on
        
        //ACTIVATE TIMER
        TA1CTL_bit.TACLR = ON; //Inicia la cuenta
        TA1CTL_bit.MC0 = ON; //Empieza la cuenta
        TA1CCTL0_bit.CCIFG = OFF; //Limpia la flag de interrupcion Timer1
        TA1CCTL0_bit.CCIE = ON; //Habilita la interrupcion Timer1
        
      }//end if(task.f2 == ON)
      
      /********************************/
      //LED2 ON
      if(task.f3 == ON)
      {
        task.f3 = OFF; //Limpia la Flag
        P1OUT_bit.P6 = LED_ON; //Enciende Led2
        
        //ACTIVATE  TIMER
        TA1CTL_bit.TACLR = ON; //Inicia la cuenta
        TA1CTL_bit.MC0 = ON; //Empieza la cuenta
        TA1CCTL0_bit.CCIFG = OFF; //Limpia la flag
        TA1CCTL0_bit.CCIE = ON;  //Habilita la interrupcion Timer1
        
      }//end if(task.f3 == ON)
      
    }//end  while(task.total != OFF)
    
  }//end for(;;)
  
}//end main(void)


/******************************************************************************/
//                            FUNCIONES
/******************************************************************************/


//CONFIG uC
//Configura el uC
void Config_uC(void)
{
  WDTCTL = WDTPW + WDTHOLD; //Para WatchDog
  DCOCTL = 0;				
  BCSCTL1 = CALBC1_1MHZ; //Frecuencia 1MHz
  DCOCTL = CALDCO_1MHZ;
}//END CONFIG uC

/*******************************************/

//CONFIG PERIPHERALS
//Configuracion de los Perifericos que vamos a usar

void Config_Peripherals(void)
{
  //LEDS
  P1DIR_bit.P0 = ON; //LED1 -> Salida
  P1DIR_bit.P6 = ON; //LED2 -> Salida
  P1OUT_bit.P0 = LED_OFF; //Apaga LED1
  P1OUT_bit.P6 = LED_OFF; //Apaga LED2
  
  //BUTTON S2
  P1DIR_bit.P3 = OFF; //P1.3 -> Entrada
  P1REN_bit.P3 = ON; //Resistencia Activa
  P1OUT_bit.P3 = ON; //Pullup Activa
  P1IES_bit.P3 = ON; //Flanco Bajada
  P1IFG_bit.P3 = OFF; //Limpia Flag
  P1IE_bit.P3 = ON; //Habilita interrupcion
  
  //TIMER 
  TA1CTL = TASSEL_2 + ID_3; //Configuramos el Timer1
  TA1CCR0 = 0x186A; //6250
  TA1CCR1 = 0xF423; //62499
  TA1CCR2 = 0xF423; //62499
  
  //USCI UA
  P1SEL_bit.P2 = 1;	//  P1.2 -> TX
  P1SEL2_bit.P2 = 1;
  UCA0CTL1_bit.UCSWRST = ON;		// Detiene USCI_A0
  UCA0CTL1_bit.UCSSEL1 = ON;		// SMCLK: 1 MHz
  UCA0BR1 = 0x00;				// 9600 baudios
  UCA0BR0 = 0x68;
  UCA0MCTL = 0x02;
  UCA0CTL1_bit.UCSWRST = OFF;		// Inicia USCI_A0    
  
  
}//END CONFIG PERIPHERALS

/*******************************************/

//INIT VARIABLES
//Inicializamos las variables a usar

void init_variables(void)
{
  task.total = 0;
  symbol = 0;
  num_interrupt = 0;
  pause = 0;
  last_pause = 0;
  c_sequence = 0;
  c_message = 0;
  
}//END INIT VARIABLES



/* -------------------------------------------------------------------------- */
/* Funcion : unsigned char Decodificar(unsigned char *secuencia)              */
/* Descripcion : Decodificacion de una secuencia de simbolos MORSE en su      */
/* correspondiente caracter                                                   */
/* Argumentos :                                                               */
/*    - secuencia: array de simbolos punto ('.') y raya ('-') terminado en    */
/*                 '\0' (maximo 5 simbolos + '\0')                            */
/* Devuelve :                                                                 */
/*    - caracter: caracter correspondiente a la secuencia de simbolos, o el   */
/*                caracter '#' si la secuencia de simbolos es erronea         */
/* Llamada desde: main                                                        */
/* Llama a :                                                                  */
/* -------------------------------------------------------------------------- */

unsigned char Decodificar(unsigned char *secuencia) {
  
/* ------------ Variables --------------------------------------------------- */
  
        const static unsigned char arbol[] = "5H4S#V3I#F#U##2E#L#R###A#P#W#J1 6B#D#X#N#C#K#Y#T7Z#G#Q#M8##O9#0";
        unsigned char caracter;
        unsigned char posicion = 31;
        unsigned char salto = 16;
        unsigned char cont_aux = 0;
/* ------------ Inicio ------------------------------------------------------ */
        while (secuencia[cont_aux] != '\0') {
        // Mientras no se alcance el caracter '\0'
                if (secuencia[cont_aux] == '.') {
                // Si el siguiente simbolo es un punto
                        posicion = posicion - salto;
                } // if (secuencia[cont_aux] == '.')
                else if (secuencia[cont_aux] == '-') {
                // Si el siguiente simbolo es una raya
                        posicion = posicion + salto;
                } // else if (secuencia[cont_aux] == '-')
                cont_aux++;
                salto=salto/2;
        }
        caracter = arbol[posicion];
        return(caracter);
} // unsigned char Decodificar(unsigned char *secuencia)



/******************************************************************************/
//                            INTERRUPTIONS
/******************************************************************************/



/*****************************************************************************/
//WATCHDOG
#pragma vector = WDT_VECTOR
__interrupt void RTI_WD(void)
{
  WDTCTL = WDTPW + WDTHOLD; //Detiene Watchdog
  P1IFG_bit.P3 = OFF; //Limpia flag
  P1IE_bit.P3 = ON; //Habilita interrupcion
}//end RTI_WD


/*****************************************************************************/
//BUTTON S2
//Rutina de tratamiento para la interrupciones del Boton S2

#pragma vector = PORT1_VECTOR
__interrupt void RTI_P1(void)
{
  P1IE_bit.P3 = OFF; //Desactiva interrupcion
  
  //Antirrebote
  WDTCTL = WDT_MDLY_32; //Configura WatchDog
  IFG1_bit.WDTIFG = OFF; //Limpia flag WD
  IE1_bit.WDTIE = ON; //Habilita interrupcion WD
  
  //Si Flanco de Bajada
  if(P1IES_bit.P3 == ON)
  {
    last_pause = pause; //almacenamos en que pausa nos encontramos
    
    //Si pausa distinto de 2 significa que estamos metiendo otro simbolo
    if(pause != 2)
    {
      //Desactivamos el Timer
      TA1CCTL2_bit.CCIE = OFF;
      TA1CTL_bit.MC1 = OFF;
        
    }//end if pause!=2
    
    
    task.f0 = ON; //Activamos esta tarea para comprobar si hemos finalizado la secuencia
    symbol = DOT; //Ponemos un punto como predeterminado a simbolo
    
    //Activamos el Timer
    TA1CTL_bit.TACLR = ON;
    TA1CTL_bit.MC1 = ON;
    TA1CCTL1_bit.CCIFG = OFF;
    TA1CCTL1_bit.CCIE = ON;
    
  }//end flanCo de bajada
  
  //Si Flanco de Subida
  else
  {
    pause = 0; //reseteamos el numero de pausas
    num_interrupt = 0; //reseteamos el num interrupciones
    
    //Si hemos pulsado un punto
    if(symbol == DOT)
    {
      task.f2 = ON; //Encendemos el LED1 durante 50ms
      
      //Desactivamos el timer
      TA1CCTL1_bit.CCIE = OFF; //Deshabilitamos la interrupcion Timer1
      TA1CTL_bit.MC1 = OFF; //Detiene Timer1
      
    }//end if(state.f0 == 0);
    
    task.f1 = 1; //Activamos la tarea de insertar secuencia correspondiente
    
    //Activamos el Timer
    TA1CTL_bit.TACLR = ON; //Inicializamos la cuenta
    TA1CTL_bit.MC1 = ON; //Comienza la Cuenta del Timer1
    TA1CCTL2_bit.CCIFG = OFF; //Limpia la flag
    TA1CCTL2_bit.CCIE = ON; //Habilita la interrupcion
    TA1CCR2 = 0xF423; //Reseteamos el valor de CCR2
    
  }//end else flanco subida
  
  P1IES_bit.P3 = ~(P1IES_bit.P3); //Conmuta el flanco
  P1IFG_bit.P3 = OFF; //limpia la flag
  
  //Si hay alguna tarea activa sal bajo consumo
  if(task.total != OFF)
  {
    __low_power_mode_off_on_exit();
  }
  
}//END PORT1_VECTOR

/************************************************************************/
//TIMER CCR0
//Con esta rutina de interrupcion contamos los 50ms de los LED

#pragma vector = TIMER1_A0_VECTOR
__interrupt void RTI_T1_TACCR0(void)
{
  P1OUT_bit.P0 = LED_OFF; //Apagamos LED1
  P1OUT_bit.P6 = LED_OFF; //Apagamos LED2
  
  //Desactivamos TIMER
  TA1CCTL0_bit.CCIE = OFF; //Deshabilita interrupcion Timer1
  TA1CTL_bit.MC0 = OFF; //Detiene Timer1
  
  //Si hay tareas sal de bajo consumo
  if(task.total)
  {
    __low_power_mode_off_on_exit();
  }
  
}//end __interrupt void RTI_T1_TACCR0(void)

/*****************************************************************************/
//TIMER CCR1
//Rutina de tratamiento de los tiempos y pausas de las pulsaciones

#pragma vector = TIMER1_A1_VECTOR
__interrupt void RTI_T1_TACCR1(void)
{
  //Si esta mas de 0.5 segundos presionado
  if(TA1CCTL1_bit.CCIE)
  {
    //Desactiva la interrupcion
    TA1CCTL1_bit.CCIE = OFF; //Desabilita la interrupcion
    TA1CTL_bit.MC1 = OFF; //detiene el Timer
    
    task.f3 = ON; //Enciende LED2
    symbol = DASH; //Asigna el simbolo Raya
    TA1CCTL1_bit.CCIFG = OFF; //Limpia la Flag
    
  }//end  if(TA1CCTL1_bit.CCIE)
  
  //Si S2 esta mas de 0.5s sin presionar
  else
  {
    num_interrupt++; //suma 1 al numero de interrupciones
    TA1CCR2 = TA1CCR2 + 0xF424; //suma 62500 al valor almacenado en TA1CCR2
    
    //Si el numero de interrupciones son 2
    //4T<=t<20T Tiempo entre caracteres
    if(num_interrupt == 2)
    {
      pause = 1; //Estamos en la primera pausa
      
      //Si el numero de simbolos es menor de 6
      if( c_sequence < 6)
      {
        sequence[c_sequence] = '\0'; //Añadimos el caracter Nulo al final
        message[c_message] = Decodificar(sequence); //Decoficamos el caracter
        UCA0TXBUF = message[c_message]; //Enviamos el caracter
        
      }//end if( c_sequence < 6)
      
      //hemos sobrepasado el numero de simbolos
      else
      {
        message[c_message] = '#'; //Añadimos el simobolo de error al mensaje
        UCA0TXBUF = message[c_message]; //mandamos el caracter
      }
      
      c_message++;//Añadimos +1 para pasar al siguiente caracter
      
    }//end if(num_interrupt == 2)
    
    //si numero de interrupciones es 10
    //Quiere decir que ha pasado 20T
    if(num_interrupt == 10)
    {
      pause = 2;//Estamos en la segunda pausa
      
      //Desactivamos el timer
      TA1CCTL2_bit.CCIE = OFF; //Deshabilitamos la interrupcion
      TA1CTL_bit.MC1 = OFF; //Detiene el Timer
      
      message[c_message] = ' '; //Añadimos un espacio al mensaje
      UCA0TXBUF = message[c_message]; //Enviamos el caracter espacio
      c_message++; //Añadimos +1 para pasar al siguiente caracter
      
    }//end if(num_interrupt == 10)
    
    TA1CCTL2_bit.CCIFG = OFF; //Limpia flas
    
  }//end else
  
  //Si hay tareas pendientes sal del bajo consumo
  if(task.total)
  {
    __low_power_mode_off_on_exit();
  }
  
}// end __interrupt void RTI_T1_TACCR1(void)