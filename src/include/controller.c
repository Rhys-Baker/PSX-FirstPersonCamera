/*
 * (C) 2024 Rhys Baker, spicyjpeg
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "controller.h"
#include "ps1/registers.h"

// All packets sent by controllers in response to a poll command include a 4-bit
// device type identifier as well as a bitfield describing the state of up to 16
// buttons.
const char *const buttonNames[] = {
	"Select",   // Bit 0
	"L3",       // Bit 1
	"R3",       // Bit 2
	"Start",    // Bit 3
	"Up",       // Bit 4
	"Right",    // Bit 5
	"Down",     // Bit 6
	"Left",     // Bit 7
	"L2",       // Bit 8
	"R2",       // Bit 9
	"L1",       // Bit 10
	"R1",       // Bit 11
	"Triangle", // Bit 12
	"Circle",   // Bit 13
	"X",        // Bit 14
	"Square"    // Bit 15
};
const char *const controllerTypes[] = {
	"Unknown",            // ID 0x0
	"Mouse",              // ID 0x1
	"neGcon",             // ID 0x2
	"Konami Justifier",   // ID 0x3
	"Digital controller", // ID 0x4
	"Analog stick",       // ID 0x5
	"Guncon",             // ID 0x6
	"Analog controller",  // ID 0x7
	"Multitap",           // ID 0x8
	"Keyboard",           // ID 0x9
	"Unknown",            // ID 0xa
	"Unknown",            // ID 0xb
	"Unknown",            // ID 0xc
	"Unknown",            // ID 0xd
	"Jogcon",             // ID 0xe
	"Configuration mode"  // ID 0xf
};

void delayMicroseconds(int time){
    // CPU is running at 33.8688 MHz
    // 1 microsecond is ~33.875 cycles
    // The loop contains a branch and decrement and thus burns 2 cycles

    time = ((time * 271) + 4) / 8;
    __asm__ volatile(
        ".set noreorder\n"
        "bgtz %0, .\n"
        "addiu %0, -2\n"
        ".set reorder\n"
        : "+r"(time)
    );
}

void initControllerBus(void){
    // Set up the serial interface with the settings used by
    // controllers and memory cards

    SIO_CTRL(0) = SIO_CTRL_RESET;

    SIO_MODE(0) = SIO_MODE_BAUD_DIV1 | SIO_MODE_DATA_8;
    SIO_BAUD(0) = F_CPU / 250000;
    // Enable TX, RX, and DSR Interrupts.
    SIO_CTRL(0) = SIO_CTRL_TX_ENABLE | SIO_CTRL_RX_ENABLE | SIO_CTRL_DSR_IRQ_ENABLE;
}

bool waitForAcknowledge(int timeout){
    // Controller and memory cards will acknowledge bytes received.
    // The send short pulses on the DSR line, then the serial interface
    // forwards them to the interrupt controller.
    // There may not be an interrupt though (E.G. If no controllers/cards are connected)
    // So we add a timeout to avoid infinite loops

    for (; timeout > 0; timeout -= 10){
        if (IRQ_STAT & (1 << IRQ_SIO0)){
            // Acknowledge / reset the IRQ and SIO flags.
            IRQ_STAT     = ~(1 << IRQ_SIO0);
            SIO_CTRL(0) |= SIO_CTRL_ACKNOWLEDGE;
            return true;
        }
        delayMicroseconds(10);
    }
    return false;
}

void selectPort(int port){
    // Set or clear the bit that controls which port we want to access
    // Controller/Memory card slot 1 or 2
    if(port){
        SIO_CTRL(0) |= SIO_CTRL_CS_PORT_2;
    } else {
        SIO_CTRL(0) &= ~SIO_CTRL_CS_PORT_2;
    }
}

uint8_t exchangeByte(uint8_t value) {
    // Wait until the device is ready to accept a byte,
    // Then wait for it to finish receiving the byte sent by the device.
    while(!(SIO_STAT(0) & SIO_STAT_TX_NOT_FULL)){
        __asm__ volatile("");
    }
    SIO_DATA(0) = value;

    while(!(SIO_STAT(0) & SIO_STAT_RX_NOT_EMPTY)){
        __asm__ volatile("");
    }
    return SIO_DATA(0);
}

int exchangePacket(
    DeviceAddress address, const uint8_t *request, uint8_t *response,
    int reqLength, int maxRespLength
) {
    // Reset the irq flag and assert the DTR signal.
    // This tell the card/controller that we are about to sent it a packet.
    // Devices may take some time to prepare for the data, so we add a small delay
    IRQ_STAT = ~(1<<IRQ_SIO0);
    SIO_CTRL(0) |= SIO_CTRL_DTR | SIO_CTRL_ACKNOWLEDGE;
    delayMicroseconds(DTR_DELAY);

    int respLength = 0;

    // Send the address byte and wait for the response from the device.
    // If no response, assume there is no connected device.
    // Otherwise, make sure the SIO data buffer is empty and prep for packet transfer.
    SIO_DATA(0) = address;

    if(waitForAcknowledge(DSR_TIMEOUT)) {
        while(SIO_STAT(0) & SIO_STAT_RX_NOT_EMPTY){
            SIO_DATA(0);
        }
        // Send and receive the packet simultaneously one byte at a time,
        // padding it with zeros if the packet we are receiving is longer than
        // the data being sent.
        while(respLength < maxRespLength){
            if(reqLength > 0){
                *(response++) = exchangeByte(*(request++));
                reqLength--;
            } else {
                *(response++) = exchangeByte(0);
            }

            respLength++;

            // The device will keep sending DSR pulses as long as there is more data to transfer.
            // If the pulses stop, terminate the transfer.
            if(!waitForAcknowledge(DSR_TIMEOUT)){
                break;
            }
        }
    }

    // Release the DSR, allowing the device to go idle.
    delayMicroseconds(DTR_DELAY);
    SIO_CTRL(0) &= ~SIO_CTRL_DTR;

    return respLength;
}

bool getControllerInfo(int port, ControllerInfo *output) {
    // Build the request packet.
    uint8_t request[4], response[8];
    //char *ptr = output;

    request[0] = CMD_POLL;  // Command
    request[1] = 0x00;      // Multitap address
    request[2] = 0x00;      // Rumble motor control 1
    request[3] = 0x00;      // Rumble motor control 2

    // Send the request to the specified controller port and grab the response.
    // This is a very slow process so only run it once per frame unless absolutely necessary
    selectPort(port);
    int respLength = exchangePacket(
        ADDR_CONTROLLER, request, response, sizeof(request), sizeof(response)
    );

    //ptr += sprintf(ptr, "Port %d:\n", port + 1);

    if(respLength < 4){
        // All controllers reply with at least 4 bytes of data.
        //ptr += sprintf(ptr, " No controller connected!");
        return false;
    }

    // The first byte of the response contains the device type ID in the upper nibble.
    // it also has the packet's payload in 2-byte units in the lower nibble.
    output->type = response[0] >> 4;

    // Bytes 2 and 3 hold a bitfield representing the state of all buttons.
    // The buttons are active-low, so invert the values.
    output->buttons = (response[2] | (response[3] << 8)) ^ 0xffff;
    output->rx = (response[4]);
    output->ry = (response[5]);
    output->lx = (response[6]);
    output->ly = (response[7]);
    return true;
}
