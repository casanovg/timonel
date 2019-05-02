/********************************************************************************

 Code Tests GC on Online C Compiler (http://www.onlinegdb.com)
 Code, Compile, Run and Debug C program online.
 Write your code in this editor and press "Run" button to compile and execute it.

*********************************************************************************/

#include <stdio.h>
#define TWI_RX_BUFFER_SIZE  32
#define TWI_RX_BUFFER_MASK (TWI_RX_BUFFER_SIZE - 1)

int data_set_A[] = { 22, 49, 48, 11, 177, 2, 99, 46, 12 };
int data_set_B[] = { 13, 33 };
int rx_buffer[TWI_RX_BUFFER_SIZE] = { 0 };
int rx_tail = 0;
int rx_head = 0;
int rx_byte_count = 0;
void ReceiveEvent(int cupin);
int UsiTwiReceiveByte (void);
void FillRxBuffer(int[], int);
void STATE_PUT_BYTE_IN_RX_BUFFER_AND_SEND_ACK(int);
void Flush(void);

int main () {
  printf ("Hello World\n\r***********\n\r");
  Flush();
  int daten_qty = sizeof(data_set_A) / sizeof(int);
  FillRxBuffer(data_set_A, daten_qty);
  int daten_qtz = sizeof(data_set_B) / sizeof(int);
  FillRxBuffer(data_set_B, daten_qtz);  
  for (int i = 0; i < daten_qty + daten_qtz; i++) {
    printf(">>> MAIN PRE rx_head: %02d >>> rx_tail: %02d >>> rx_byte_count: %02d [cupin: %02d]\r\n", rx_head, rx_tail, rx_byte_count, rx_head - rx_tail);
    printf(">>> MAIN RECEIVE EVENT rx_head: %02d >>> rx_tail: %02d >>> rx_byte_count: %02d >>> RX buffer data: %d\r\n", rx_head, rx_tail, rx_byte_count, UsiTwiReceiveByte());
  }
  //ReceiveEvent(rx_head - rx_tail);
  return 0;
}

// Function ReceiveEvent
//void ReceiveEvent(int cupin) {
//      for (int i = 0; i < cupin; i++) {
//      printf(">>> RECEIVE EVENT rx_head: %02d >>> rx_tail: %02d >>> rx_byte_count: %02d [cupin: %02d] >>> RX buffer data: %d\r\n", rx_head, rx_tail, rx_byte_count, cupin, UsiTwiReceiveByte());
//    }
//}

// Function Flush
void Flush(void) {
    rx_tail = rx_head = rx_byte_count = 0;
    //rx_tail = rx_head = 0;
}

// Function UsiTwiReceiveByte
int UsiTwiReceiveByte (void) {
  while(rx_head == rx_tail) {};
  //while(!rx_byte_count--) {};
  rx_byte_count--;
  rx_tail = ((rx_tail + 1) & TWI_RX_BUFFER_MASK);	/* Update the RX buffer index */
  printf(">>> USITWIRECEIVEBYTE rx_head: %02d >>> rx_tail (IX): %02d >>> rx_byte_count: %02d \r\n", rx_head, rx_tail, rx_byte_count);
  return rx_buffer[rx_tail];	/* Return data from the buffer */
}

// Function PUT_BYTE_IN_RX_BUFFER
void STATE_PUT_BYTE_IN_RX_BUFFER_AND_SEND_ACK (int USIDR) {
  rx_byte_count++;
  rx_head = ((rx_head + 1) & TWI_RX_BUFFER_MASK);
  rx_buffer[rx_head] = USIDR;
  printf(">>> PUT_BYTE_IN_RX_BUFFER rx_head (IX): %02d >>> rx_tail: %02d >>> rx_byte_count: %02d >>> RX buffer data: %d\r\n", rx_head, rx_tail, rx_byte_count, rx_buffer[rx_head]);  
}

// Function FillRxBuffer
void FillRxBuffer (int data_set[], int data_size) {
  //printf("data size: %d\n\r", data_size);
  for (int i = 0; i < data_size; i++) {
      STATE_PUT_BYTE_IN_RX_BUFFER_AND_SEND_ACK (data_set[i]);
    }
}


