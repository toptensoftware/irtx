#include <Arduino.h>
#include "ir_encoder.h"

void ir_encode(const IrProtocol& protocol, uint64_t code, bool repeat, int* signal, int* count)
{
    // Repeat code?
    if (repeat && protocol.repeat[0] != '\0')
    {
        signal[0] = protocol.repeat[0];
        signal[1] = protocol.repeat[1];
        signal[2] = protocol.repeat[2];
        signal[3] = protocol.repeat[3];
        *count = 4;
        return;
    }

    int index = 0;

    // Header
    signal[index++] = protocol.header.pulse;
    signal[index++] = protocol.header.space;

    // Data bits
    for (int i = protocol.bitCount - 1; i >= 0; i--)
    {
        bool bit = (code >> i) & 1;
        if (bit)
        {
            signal[index++] = protocol.one.pulse;
            signal[index++] = protocol.one.space;
        }
        else
        {
            signal[index++] = protocol.zero.pulse;
            signal[index++] = protocol.zero.space;
        }
    }

    // Footer
    signal[index++] = protocol.footer.pulse;

    // Trailing gap or frame length
    if(protocol.frameLength)
    {
        int length = 0;
        for (int i=0; i<index; i++)
        {
            length += signal[i];
        }
        signal[index++] = protocol.frameLength - length;
    }
    else
    {
        signal[index++] = protocol.footer.space;
    }

    // Return count
    *count = index;
}


void ir_encode(const IrProtocol& protocol, uint64_t code, bool repeat, uint16_t* signal, int* count, int* pGap)
{
    // Plain encode
    int signal32[128];
    ir_encode(protocol, code, repeat, signal32, count);

    // Pack down to 16-bit and extract final gap (which may not fit in 16 bits)
    *count--; // Exclude the final gap from count
    for (int i = 0; i < *count; i++)
    {
        signal[i] = (uint16_t)signal32[i];
    }
    *pGap = signal32[*count];
}