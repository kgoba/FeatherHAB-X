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

def compress91(value, n_digits):
    value = int(0.5 + value)
    result = bytearray()
    for idx in range(n_digits):
        d = value % 91
        result += chr(33 + d)
        value /= 91
    result.reverse()
    return result

def format_nmea_lat(degrees):
    """ '5102.56N' """
    if degrees >= 0: 
        degrees_abs = degrees
        cardinal = 'N'
    else: 
        degrees_abs = -degrees
        cardinal = 'S'
    deg_int = int(degrees_abs)
    minutes = 60 * (degrees_abs - deg_int)
    return '%02d%05.2f%s' % (deg_int, minutes, cardinal)

def format_nmea_lng(degrees):
    """ '02546.56E' """
    if degrees >= 0: 
        degrees_abs = degrees
        cardinal = 'E'
    else: 
        degrees_abs = -degrees
        cardinal = 'W'
    deg_int = int(degrees_abs)
    minutes = 60 * (degrees_abs - deg_int)
    return '%03d%05.2f%s' % (deg_int, minutes, cardinal)

def encode_position(symbol_code, lat, lng, 
        alt = None, speed = None, course = None, 
        symbol_table = '/', timestamp = None, comment = None, 
        with_messaging = False, compressed = True):
        
    """ APRS lat/lon position report with optional altitude and/or course/speed """
    result = bytearray()
    if timestamp != None: 
        # Position with timestamp
        if with_messaging:
            result += '@' # with APRS messaging
        else:
            result += '/' # without APRS messaging
        result += timestamp  # 234517h   (23:45:17 UTC)
    else: 
        # Position without timestamp 
        if with_messaging:
            result += '!' # (no APRS messaging)
        else:
            result += '=' # (with APRS messaging)

    if compressed:
        result += symbol_table
        result += compress91(380926 * (90 - lat), 4)
        result += compress91(190463 * (180 + lng), 4)
        result += symbol_code
        if alt != None:
            if alt > 0:
                alt_feet = alt / 0.3048
                result += compress91(math.log(alt_feet) / math.log(1.002), 2)
            else:
                result += compress91(1, 2)
            result += chr(33 + 0x30) # 00FNNOOO   F=1(current fix) NN=10(GGA) OOO=000(Compressed)
        elif course != None and speed != None:
            course_enc = int(0.5 + course / 4)
            speed_knots = speed * (3600 / 1852.0)
            speed_enc = int(0.5 + math.log(1 + speed_knots) / math.log(1.08))
            result += chr(33 + course_enc)
            result += chr(33 + speed_enc)
            result += chr(33 + 0x38) # 00FNNOOO   F=1(current fix) NN=11(RMC) OOO=000(Compressed)
        else:
            # no altitude or course/speed data
            result += '   '
    else:
        result += format_nmea_lat(lat)
        result += symbol_table
        result += format_nmea_lng(lng)
        result += symbol_code
        if course != None and speed != None:
            result += '%03d/%03d' % (course, speed)
        if alt != None: 
            if alt > 0:
                alt_feet = int(0.5 + alt / 0.3048)
            else:
                alt_feet = 0
            result += '/A=%06d' % alt_feet
            if comment:
                result += ' '

    if comment: result += comment
    return result

def break_degrees(degrees):
    if degrees >= 0: 
        degrees_abs = degrees
    else: 
        degrees_abs = -degrees
    deg_int = int(degrees_abs)
    minutes = 60 * (degrees_abs - deg_int)
    min_int = int(minutes)
    min_100 = int(0.5 + 100 * (minutes - min_int))
    return (deg_int, min_int, min_100)

def encode_mice_digit(digit, alt = False, custom = False):
    result = bytearray()
    if alt:
        if custom:
            result += chr(digit + ord('A'))
        else:
            result += chr(digit + ord('P'))
    else:
        result += chr(digit + ord('0'))
    return result
    
def encode_mice(symbol_code, lat, lng, 
        alt = None, speed = None, course = None, 
        symbol_table = '/', 
        msg_type = 7, custom_msg = False, status = None):
    """ MESSAGE TYPES:
        0 - EMERGENCY
        1 - M6: Priority
        2 - M5: Special
        3 - M4: Committed
        4 - M3: Returning
        5 - M2: In Service
        6 - M1: En Route
        7 - M0: Off Duty
    """
    d_call = bytearray()

    (lat_int, lat_min, lat_100) = break_degrees(lat)
    (lng_int, lng_min, lng_100) = break_degrees(lng)
    print lng_int, lng_min, lng_100
        
    lng_offset = (lng_int < 10) or (lng_int >= 100)
    
    msg_a = (msg_type & 4) > 0
    msg_b = (msg_type & 2) > 0
    msg_c = (msg_type & 1) > 0
    
    d_call += encode_mice_digit(lat_int / 10, msg_a, custom_msg)
    d_call += encode_mice_digit(lat_int % 10, msg_b, custom_msg)
    d_call += encode_mice_digit(lat_min / 10, msg_c, custom_msg)
    d_call += encode_mice_digit(lat_min % 10, lat >= 0)
    d_call += encode_mice_digit(lat_100 / 10, lng_offset)
    d_call += encode_mice_digit(lat_100 % 10, lng < 0)

    result = bytearray()
    result += '\x60'        # GRAVE ACCENT
    if lng_int < 10:
        result += chr(118 + lng_int)
    elif lng_int < 100:
        result += chr(28 + lng_int)
    elif lng_int < 110:
        result += chr(108 + (lng_int - 100))
    else:
        result += chr(28 + (lng_int - 100))
    
    if lng_min < 10:
        result += chr(88 + lng_min)
    else:
        result += chr(28 + lng_min)
    
    result += chr(28 + lng_100)

    if speed != None:
        speed_knots = int(0.5 + speed * (3600 / 1852.0))
    else:
        speed_knots = 0
    if course != None:
        course_100 = int(course / 100)
        course_d2  = int(course - 100 * course_100)
    else:
        course_100 = 0
        course_d2  = 0

    speed_tens = int(speed_knots / 10)
    speed_ones = speed_knots - (10 * speed_tens)

    result += chr(28 + speed_tens)
    result += chr(32 + speed_ones * 10 + course_100)
    result += chr(28 + course_d2)
    
    result += symbol_code
    result += symbol_table
    
    if alt != None:
        result += compress91(alt + 10000, 3)
        result += '}'
        
    if status != None:
        result += status
    
    return (d_call, result)
    
    
SYM_BALLOON = 'O'
# 56.83385,25.78354,318

(d_call, data) = encode_mice(SYM_BALLOON, 56.82566, 25.77608, alt = 194, speed = 10, course = 234, status = 'ROLLING GOOD')
addressList = [(d_call, 1), ('YL3AJG', 1)]

#addressList = [('APT310', 0), ('YL3AJG', 0), ('WIDE1', 1), ('WIDE2', 1)]
#addressList = [('GPS', 0), ('YL3AJG', 0), ('WIDE2', 1)]
#addressList = [('GPS', 1), ('YL3AJG', 1)]
#data = encode_position(SYM_BALLOON, 56.82566, 25.77608, 194)
#data = encode_position('>', 49.5, -72.75, 3049)

#data = '=4603.63N/01431.26E-Op. Andrej'
#data = '=5102.56N/00343.35E-Op. Karlis'

print addressList, ':', data

frame = getAX25Frame(addressList, data)
bits = getAX25Bits(frame)

print len(bits), 'bits'
print frame
print ' '.join(['%02x' % x for x in frame])
print ''.join(['%d' % x for x in bits])

flag = bytearray( (0, 1, 1, 1, 1, 1, 1, 0) )

bits = flag * 50 + bits + flag * 5

pcm = getBellPCM(bits, 48000)
pcm.extend([0] * 48000 * 2)

#print ' '.join(['%d' % (32767 * x) for x in pcm])

savePCM(pcm, 48000, 'out.wav')
