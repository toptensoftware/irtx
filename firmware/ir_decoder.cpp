#include <Arduino.h>
#include <vector>
#include "ir_decoder.h"

IrDecoder::IrDecoder(const IrProtocol& protocol, PFNIRDECODE handler)
    : m_protocol(protocol), m_handler(handler)
{
}

void IrDecoder::update(bool pulse, int length)
{
    update_decode_code(pulse, length);
    update_decode_repeat(pulse, length);
}



void IrDecoder::update_decode_code(bool pulse, int length)
{   
    //Serial.printf("  - %s %i/%i %s %i\n", m_protocol.name, (int)m_codeState, m_bitCount, pulse ? "pulse": "space",  length);
    switch (m_codeState)
    {
        case CodeState::Initial:
            if (pulse && match(length, m_protocol.header.pulse))
            {
                m_codeState = CodeState::HaveHeaderPulse;
            }
            break;

        case CodeState::HaveHeaderPulse:
            if (!pulse && match(length, m_protocol.header.space))
            {
                m_codeState = CodeState::StartBit;
                m_data = 0;
                m_bitCount = 0;
            }
            else
            {
                m_codeState = CodeState::Initial;
            }
            break;

        case CodeState::StartBit:
            if (pulse && m_bitCount == m_protocol.bitCount)
            {
                //Serial.printf("  - received data: %llx\n", m_data);

                // Matched
                m_handler(m_protocol, m_data);
                    
                m_codeState = CodeState::Initial;
            }
            else if (pulse)
            {
                if (match(length, m_protocol.one.pulse))
                {
                    if (match(length, m_protocol.zero.pulse))
                        m_codeState = CodeState::InEitherBit;
                    else
                        m_codeState = CodeState::In1Bit;
                }
                else if (match(length, m_protocol.zero.pulse))
                {
                    m_codeState = CodeState::In0Bit;
                }
                else
                {
                    m_codeState = CodeState::Initial;
                }
            }
            else
            {
                m_codeState = CodeState::Initial;
            }
            break;

        case CodeState::InEitherBit:
            if (!pulse && match(length, m_protocol.one.space))
            {
                m_data = (m_data << 1) | 1;
                m_bitCount++;
                m_codeState = CodeState::StartBit;
            }
            else if (!pulse && match(length, m_protocol.zero.space))
            {
                m_data <<= 1;
                m_bitCount++;
                m_codeState = CodeState::StartBit;
            }
            else
            {
                m_codeState = CodeState::Initial;
            }
            break;

        case CodeState::In1Bit:
            if (!pulse && match(length, m_protocol.one.space))
            {
                m_data = (m_data << 1) | 1;
                m_bitCount++;
                m_codeState = CodeState::StartBit;
            }
            else
            {
                m_codeState = CodeState::Initial;
            }
            break;

        case CodeState::In0Bit:
            if (!pulse && match(length, m_protocol.zero.space))
            {
                m_data <<= 1;
                m_bitCount++;
                m_codeState = CodeState::StartBit;
            }
            else
            {
                m_codeState = CodeState::Initial;
            }
            break;
    }
}


void IrDecoder::update_decode_repeat(bool pulse, int length)
{
    switch (m_repeatState)
    {
        case RepeatState::ExpectPulse:
            if (pulse && match(length, m_protocol.repeat[m_repeatPos]))
            {   
                m_repeatPos++;
                m_repeatState = RepeatState::ExpectSpace;
                if (m_repeatPos == 3)
                {
                    m_handler(m_protocol, 0);
                    m_repeatState = RepeatState::ExpectPulse;
                    m_repeatPos = 0;
                }
            }
            else
            {
                m_repeatPos = 0;
                m_repeatState = RepeatState::ExpectPulse;
            }
            break;

        case RepeatState::ExpectSpace:
            if (!pulse && match(length, m_protocol.repeat[m_repeatPos]))
            {   
                m_repeatState = RepeatState::ExpectPulse;
                m_repeatPos++;
            }
            else
            {
                m_repeatState = RepeatState::ExpectPulse;
                m_repeatPos = 0;    
            }
            break;
    }
}

/*
bool IrDecoder::match(int length, int expected)
{
    bool r = match2(length, expected);
    Serial.printf("  - compare %i to %i = %s\n", length, expected, r ? "match" : "mismatch");
    return r;
}
*/

bool IrDecoder::match(int length, int expected)
{
    if (expected == 0)
        return false;

    return length > expected * 0.6 && length < expected * 1.4;  
}



static std::vector<IrDecoder*> s_decoders;

// Initialize decoders for all registered protocols
void ir_decode_init(PFNIRDECODE handler)
{
    int count = getIrProtocolCount();
    for (int i = 0; i < count; i++)
    {
        s_decoders.push_back(new IrDecoder(*getIrProtocolByIndex(i), handler));
    }
}

// Update decoders with new pulse/space
void ir_decode(bool pulse, int length)
{
    for (IrDecoder* decoder : s_decoders)
    {
        decoder->update(pulse, length);
    }
}
