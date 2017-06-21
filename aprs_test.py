#!/usr/bin/env python

import sys
import math
import wave
import struct

class AX25Frame:
    def __init__(self):
        pass
    
    def setDestination(self, destination):
        pass
        
def getAX25CRC(frame):
    crc = 0xFFFF
    for byte in frame:
        for idxBit in range(8):
            bit = byte & 1
            byte >>= 1
            crc ^= bit
            if crc & 1:
                crc = (crc >> 1) ^ 0x8408   # X-modem CRC poly
            else:
                crc = (crc >> 1)
    return crc

def getAX25Bits(frame):
    onesInRow = 0
    bits = bytearray()
    
    for byte in frame:
        for idxBit in range(8):
            bit = byte & 1
            byte >>= 1
            if bit:
                bits.append(1)
                onesInRow += 1
                if onesInRow < 5:
                    continue

            bits.append(0)
            onesInRow = 0
    print len(frame) * 8, len(bits)
    return bits

def getAX25Frame(addressList, data):
    frame = bytearray()
    
    for (idxAddress, (call, ssid)) in enumerate(addressList):
        if len(call) < 6:
            call += ' ' * (6 - len(call))
        
        # Transmit callsign + pad
        for x in bytearray(call):
            frame.append(x << 1)
        
        # Transmit SSID. Termination signaled with last bit = 1
        ssidByte = ord('0') + ssid
        if idxAddress == (len(addressList) - 1):
            frame.append( (ssidByte << 1) | 1)
        else:
            frame.append( ssidByte << 1 )
    
    # Control field: 3 = APRS-UI frame
    frame.append(0x03)
    
	# Protocol ID: 0xf0 = no layer 3 data
    frame.append(0xf0);

    frame.extend(data)
    
    crc = getAX25CRC(frame)
    
    frame.append(0xff ^ (crc & 0xff))
    frame.append(0xff ^ ((crc >> 8) & 0xff))
    
    return frame
    
def getBellPCM(bits, samplerate, freqMark = 1200, freqSpace = 2200, baud = 1200):
    deltaT = 1.0 / samplerate
    deltaPhaseMark = 2 * math.pi * freqMark * deltaT
    deltaPhaseSpace = 2 * math.pi * freqSpace * deltaT

    signal = []
    phase = 0
    phaseType = 1

    bitIdx = 0
    timeTotal = len(bits) / float(baud)
    timeNext = 1.0 / baud
    time = 0
    while time < timeTotal:
        if time >= timeNext and bitIdx < len(bits):
            timeNext += 1.0 / baud
            bit = bits[bitIdx]
            bitIdx += 1
            if bit == 0:
                phaseType = 1 - phaseType
            
        signal.append(math.sin(phase))
            
        if phaseType == 1:
            phase += deltaPhaseMark
        else:
            phase += deltaPhaseSpace

        time += deltaT

    return signal

def savePCM(pcm, samplerate, filename):
    f = wave.open(filename, 'w')
    
    f.setnchannels(1)
    f.setsampwidth(2)
    f.setframerate(samplerate)

    pcmRaw = [int(0.5 + 32767 * x) for x in pcm]
    frames = struct.pack('<' + 'h' * len(pcm), *pcmRaw)

    f.writeframes(frames)
    f.close()
    pass

addressList = [('APT310', 0), ('YL3AJG', 0), ('WIDE1', 1), ('WIDE2', 1)]
#addressList = [('APRS', 0), ('YL3AJG', 0), ('WIDE2', 1)]
data = '!'
data += '5102.56N/00343.35E'
data += '>' # O for balloon?
data += '000/000'
data += '/A=000123'
#data += '/T=12'
#data += '/HAB Propagation Test'

#data = '=4603.63N/01431.26E-Op. Andrej'
#data = '=5102.56N/00343.35E-Op. Karlis'


frame = getAX25Frame(addressList, data)
bits = getAX25Bits(frame)

print frame
print ' '.join(['%02x' % x for x in frame])
print ''.join(['%d' % x for x in bits])

flag = bytearray( (0, 1, 1, 1, 1, 1, 1, 0) )

bits = flag * 50 + bits + flag * 5

pcm = getBellPCM(bits, 48000)
pcm.extend([0] * 48000 * 2)

#print ' '.join(['%d' % (32767 * x) for x in pcm])

savePCM(pcm, 48000, 'out.wav')
