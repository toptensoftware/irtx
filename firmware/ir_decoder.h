#pragma once

#include <stdint.h>
#include "ir_protocol.h"

typedef void (*PFNIRDECODE)(const IrProtocol& protocol, uint64_t data);

class IrDecoder
{
public:
    IrDecoder(const IrProtocol& protocol, PFNIRDECODE handler);

    void update(bool pulse, int length);

private:
    
    const IrProtocol& m_protocol;
    PFNIRDECODE m_handler;

    // Code decoding
    void update_decode_code(bool pulse, int length);

    enum class CodeState
    {
        Initial,
        HaveHeaderPulse,
        StartBit,
        InEitherBit,
        In1Bit,
        In0Bit
    };

    CodeState m_codeState = CodeState::Initial;
    uint64_t m_data = 0;
    int m_bitCount = 0;

    // Repeat decoding
    void update_decode_repeat(bool pulse, int length);
    enum class RepeatState
    {
        ExpectPulse,
        ExpectSpace,
    };

    RepeatState m_repeatState = RepeatState::ExpectPulse;
    int m_repeatPos = 0;


    static bool match(int length, int expected);
    //static bool match2(int length, int expected);
};


// Initialize decoders for all registered protocols
void ir_decode_init(PFNIRDECODE handler);

// Update decoders with new pulse/space
void ir_decode(bool pulse, int length);
