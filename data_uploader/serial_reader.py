import serial
import json
import xml.etree.ElementTree as ET

debug = True

if debug:
    # x = input("Please input the port number:")
    string = b'2,3,0,4,2,\r\n'
else:
    car = serial.Serial("COM9")
    print("connected to: " + car.name)


cargo = [0 for i in range(4)]


def append_xml(goodID, goodName, position, type, Robot_robotID):
    tree = ET.parse('./cargo.xml')
    root = tree.getroot()
    cargo = ET.SubElement(root, 'cargo')
    cargo.set('goodID', goodID)
    ET.SubElement(cargo, 'goodName').text = goodName
    ET.SubElement(cargo, 'position').text = position
    ET.SubElement(cargo, 'type').text = type
    ET.SubElement(cargo, 'Robot_robotID').text = Robot_robotID
    tree.write('./cargo.xml')


def int_2_alphabet(a):
    b = int(a/26)
    c = a % 26
    return str(chr(b+65))+str(chr(c+65))


while True:
    if debug:
        count, cargo[0], cargo[1], cargo[2], cargo[3], additional = string.decode().split(
            ',')
    else:
        count, cargo[0], cargo[1], cargo[2], cargo[3], additional = car.readline(
        ).decode().split(',')

    count = int(count)
    cargo = list(map(int, cargo))
    # raw = {"count": count, "cargo": cargo, "additional": additional}
    # data = json.dumps(raw)
    for i in range(len(cargo)):
        append_xml(str(count*10+i), "idontknow", str(i), str(cargo[i]), "001")

    print(cargo)

car.close()