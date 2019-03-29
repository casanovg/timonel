
/************************************************************
! Corrected version of the VAR Appnote for USI I2C Interface !
!!! ONLY parts to be changed are listed here !!!
************************************************************/

/*********************************************************************************/
/*** USI INIT                                                          ***/
/*********************************************************************************/
 PORT_USI |=  (1<<PORT_USI_SCL);                                 // Set SCL high
 PORT_USI |=  (1<<PORT_USI_SDA);                                 // Set SDA high
 DDR_USI  |=  (1<<PORT_USI_SCL);                                 // Set SCL as output
 DDR_USI  &= ~(1<<PORT_USI_SDA);                                 // Set SDA as input
 USICR    =  (1<<USISIE)|(0<<USIOIE)|                            // Enable Start Condition Interrupt. Disable Overflow Interrupt.
             (1<<USIWM1)|(1<<USIWM0)|                            // Set USI in Two-wire mode. No USI Counter overflow prior
                                                                  // to first Start Condition (potentail failure)
             (1<<USICS1)|(0<<USICS0)|(0<<USICLK)|                // Shift Register Clock Source = External, positive edge
             (0<<USITC);
  USISR    = 0xF0;                                                // Clear all flags and reset overflow counter
  USI_TWI_Overflow_State=USI_SLAVE_StateNone; 

/*********************************************************************************/
/*** USI START cond.                                                           ***/
/*********************************************************************************/
ISR(SIG_USI_START)
{
    unsigned char tmpUSISR;                                         // Temporary variable to store volatile
    
    tmpUSISR = USISR;                                               // Not necessary, but prevents warnings
    // Set default starting conditions for new TWI package
    USI_TWI_Overflow_State = USI_SLAVE_CHECK_ADDRESS;
    DDR_USI  &= ~(1<<PORT_USI_SDA);                                 // Set SDA as input

    /** NEW delay until both pins are low **/	
    while ( (PIN_USI & (1<<PORT_USI_SCL)) && (!(PIN_USI & (1<<PORT_USI_SDA))));
    /** NEW delay until both pins are low **/

    if (!( PIN_USI & ( 1 << PIN_USI_SDA )))
    {
           // NO Stop
      USICR   = (1<<USISIE)|(1<<USIOIE)|                            // Enable Overflow and Start Condition Interrupt. (Keep StartCondInt to detect RESTART)
                (1<<USIWM0)|(1<<USIWM1)|                            // Set USI in Two-wire mode.
                (1<<USICS1)|(0<<USICS0)|(0<<USICLK)|                // Shift Register Clock Source = External, positive edge
                (0<<USITC);
    }
    else
    {     // STOP
      USICR   = (1<<USISIE)|(0<<USIOIE)|                            // Enable Overflow and Start Condition Interrupt. (Keep StartCondInt to detect RESTART)
                (1<<USIWM1)|(0<<USIWM1)|                            // Set USI in Two-wire mode.
                (1<<USICS1)|(0<<USICS0)|(0<<USICLK)|                // Shift Register Clock Source = External, positive edge
                (0<<USITC);
    
    }
    USISR  =    (1<<USI_START_COND_INT)|(1<<USIOIF)|(1<<USIPF)|(1<<USIDC)|      // Clear flags
                (0x0<<USICNT0);                                     // Set USI to sample 8 bits i.e. count 16 external pin toggles.    

}

 
/*********************************************************************************/
/*** USI CNTer overflow -> New Data                                            ***/
/*********************************************************************************/
ISR(SIG_USI_OVERFLOW)
{
  unsigned int t16;
  
  switch (USI_TWI_Overflow_State)
  {
    // ---------- Address mode ----------
    // Check address and send ACK (and next USI_SLAVE_SEND_DATA) if OK, else reset USI.
    case USI_SLAVE_CHECK_ADDRESS:
      if ((USIDR == 0) || (( USIDR>>1 ) == TWI_slaveAddress))
      {
        
        if ( USIDR & 0x01 )
        {
          USI_TWI_Overflow_State = USI_SLAVE_SEND_DATA;
        }
        else
        {
          USI_TWI_Overflow_State = USI_SLAVE_REQUEST_DATA;
        }
        SET_USI_TO_SEND_ACK();
          
      }
      else
      {
        SET_USI_TO_TWI_START_CONDITION_MODE();          // not my Adress.....
      }
      break;


    // ----- Master write data mode ------
  

