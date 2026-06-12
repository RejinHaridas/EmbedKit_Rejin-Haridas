/* Wrap the index using a bit mask instead of the modulo (%) operator.
 *
 * On many microcontrollers, a bitwise AND operation can be executed in a
 * single CPU instruction, whereas modulo may require a division operation.
 * MCUs without a hardware divider often perform division in software,
 * making modulo significantly more expensive in terms of CPU cycles.
 * So the Bitwise AND operation is used instead of Modulo operator because
 * it uses minimum CPU cycles
 * This optimization is only valid when BUFFER_SIZE is a power of 2
 * (e.g., 8, 16, 32, 64). In such cases:
 *
 *      index % BUFFER_SIZE
 *
 * is equivalent to:
 *
 *      index & (BUFFER_SIZE - 1)
 *
 * allowing efficient wraparound of the ring buffer index.
 */

#include<stdio.h>
#include<stdint.h>
#include<stdbool.h>

#define BUFFER_SIZE 8

/* Return codes for ring buffer operations */
typedef enum
{
        RB_OK = 0,  /* Operation successful */
        RB_FULL,    /* Write failed: buffer is full */
        RB_EMPTY    /* Read failed: buffer is empty */
}RB_Status_t;

/* Structure representing the Ring Buffer */
typedef struct
{
        uint8_t RingBuffer[BUFFER_SIZE];
        uint8_t head;
        uint8_t tail;
        uint8_t count;
}Ringbuffer_t;


/* Initialize the Ring Buffer to an empty state */
void RingBuffer_Init(Ringbuffer_t *rb)
{
        rb->head        =   0 ;
        rb->tail        =   0 ;
        rb->count       =   0 ;
}


/* Check whether the Ring Buffer is empty */
bool RingBuffer_IsEmpty(const Ringbuffer_t *rb)
{
        return(rb->count == 0);
}


/* Check whether the Ring Buffer is full */
bool RingBuffer_IsFull(const Ringbuffer_t *rb)
{
        return(rb->count == BUFFER_SIZE);
}


/*
 * Write a single byte into the Ring Buffer.
 * Uses bit masking instead of modulo for index wraparound.
 * This is faster on many MCUs and works because BUFFER_SIZE is a power of 2.
 */
RB_Status_t RingBuffer_Write(Ringbuffer_t *rb,uint8_t data)
{
        if(RingBuffer_IsFull(rb))
        {
                return RB_FULL;
        }

        rb->RingBuffer[rb->head] = data;
        rb->head = (rb->head + 1) & (BUFFER_SIZE - 1);

        rb->count++;

        return RB_OK;
}


/*
 * Read a single byte from the Ring Buffer.
 * Uses bit masking instead of modulo for index wraparound.
 */
RB_Status_t RingBuffer_Read(Ringbuffer_t *rb,uint8_t *data)
{
        if(RingBuffer_IsEmpty(rb))
        {
                return RB_EMPTY;
        }

        *data = rb->RingBuffer[rb->tail];
        rb->tail = (rb->tail + 1) & (BUFFER_SIZE - 1);

        rb->count--;

        return RB_OK;
}


/* Demonstrate all required ring buffer operations */
int main()
{
        Ringbuffer_t rb;

        RingBuffer_Init(&rb);

        /* Initial data used to fill the buffer */
        uint8_t initial_data[8] = {0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48 };


        /* Operation 1: Fill the buffer with 8 bytes */
        printf("======== Operation 1 ========\n\n");
        printf("BUFFER INITIALIZATION\n\n");

        for(uint8_t i = 0; i < BUFFER_SIZE; i++)
        {
                if(RingBuffer_Write(&rb,initial_data[i]) == RB_OK)
                {
                        if(rb.count == BUFFER_SIZE)
                        {
                                printf("[WRITE] 0x%02X -> OK (count = %d) [FULL]\n", initial_data[i],rb.count);
                        }
                        else
                        {
                                printf("[WRITE] 0x%02X -> OK (count = %d)\n", initial_data[i],rb.count);
                        }
                }
                else
                {
                        printf("[WRITE] 0x%02X -> FAIL (count = %d)\n", initial_data[i],rb.count);
                }
        }

        printf("\nBuffer Full : %s\n", RingBuffer_IsFull(&rb) ? "YES" : "NO");

        printf("Count: %d\n\n", rb.count);


        /* Operation 2: Verify write failure when buffer is full */
        printf("======== Operation 2 ========\n");
        printf("\nAttempt to write when buffer is full\n\n");

        uint8_t data = 0x99;

        if(RingBuffer_Write(&rb,data) == RB_FULL)
        {
                printf("[WRITE] 0x%02X -> FAIL (Buffer Full)\n\n",data);
        }
        else
        {
                printf("[WRITE] 0x%02X -> OK (count = %d)\n\n",data,rb.count);
        }

        printf("Buffer Empty : %s\n", RingBuffer_IsEmpty(&rb) ? "YES" : "NO");

        printf("Count : %u\n\n", rb.count);


        /* Operation 3: Read 3 bytes from the buffer */
        printf("======== Operation 3 ========\n");
        printf("\nRead 3 bytes\n\n");

        for(int i = 0; i < 3; i++)
        {
                if(RingBuffer_Read(&rb,&data) == RB_OK)
                {
                        printf("[READ] -> 0x%02X (count = %d)\n",data,rb.count);
                }
        }

        printf("\nBuffer Empty : %s\n",RingBuffer_IsEmpty(&rb) ? "YES" : "NO");

        printf("Count : %u\n\n", rb.count);


        /* Operation 4: Reuse freed slots by writing 3 new bytes */
        printf("======== Operation 4 ========\n");
        printf("\nWrite 3 bytes\n\n");

        uint8_t new_data[3] = {0x49,0x4A,0x4B};

        for(int i = 0; i < 3; i++)
        {
                if(RingBuffer_Write(&rb,new_data[i]) == RB_OK)
                {
                        if(rb.count == BUFFER_SIZE)
                        {
                                printf("[WRITE] 0x%02X -> OK (count = %d) [FULL]\n", new_data[i],rb.count);
                        }
                        else
                        {
                                printf("[WRITE] 0x%02X -> OK (count = %d)\n", new_data[i],rb.count);
                        }
                }
        }

        printf("\nBuffer Empty : %s\n", RingBuffer_IsEmpty(&rb) ? "YES" : "NO");

        printf("Count : %u\n\n", rb.count);


        /* Operation 5: Read all remaining bytes until buffer is empty */
        printf("======== Operation 5 ========\n");
        printf("\nRead remaining bytes\n\n");

        while(!RingBuffer_IsEmpty(&rb))
        {
                if(RingBuffer_Read(&rb,&data) == RB_OK)
                {
                        printf("[READ] -> 0x%02X (count = %d)\n",
                                data,rb.count);
                }
        }

        printf("\nBuffer Empty : %s\n",RingBuffer_IsEmpty(&rb) ? "YES" : "NO");

        printf("Count : %u\n\n", rb.count);


        /* Operation 6: Verify read failure when buffer is empty */
        printf("======== Operation 6 ========\n");
        printf("\nAttempt to read from empty buffer\n\n");

        if(RingBuffer_Read(&rb,&data) == RB_EMPTY)
        {
                printf("[READ] -> FAIL (Buffer Empty)\n");
        }

        printf("\nBuffer Empty : %s\n",RingBuffer_IsEmpty(&rb) ? "YES" : "NO");

        printf("Count : %u\n\n", rb.count);

        return 0;
}

