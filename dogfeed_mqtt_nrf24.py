import RPi.GPIO as GPIO
from lib_nrf24 import NRF24
import time
import spidev
import paho.mqtt.client as paho
import arrow
import logging
from logging.config import fileConfig

fileConfig('logging_config.ini')
logger = logging.getLogger()

logger.info('Dogfeed MQTT NRF24 started')

def on_connect(client, userdata, flags, rc):
    print("CONNACK received with code {}.".format(rc))

def on_publish(client, userdata, mid):
    print("mid: {}".format(mid))

client = paho.Client()
client.username_pw_set("adafruit", "adafruit")
client.connect("wiseapp.xyz", 1883)
client.loop_start()

GPIO.setmode(GPIO.BCM)

pipes = [[0xE8, 0xE8, 0xF0, 0xF0, 0xE1], [0xF0, 0xF0, 0xF0, 0xF0, 0xE1]]

radio = NRF24(GPIO, spidev.SpiDev())
radio.begin(0, 17)

radio.setPayloadSize(32)
radio.setChannel(0x76)
radio.setDataRate(NRF24.BR_1MBPS)
radio.setPALevel(NRF24.PA_MIN)

#radio.setAutoAck(True)
radio.enableDynamicPayloads()
#radio.enableAckPayload()

radio.openReadingPipe(1, pipes[1])
radio.printDetails()
radio.startListening()

def display_output(translated_string):
    print("Out received message decodes to: {}".format(translated_string), end='\r')


def translate_message(receivedMessage):
    result_string = ""
    for n in receivedMessage:
        # Decode into standard unicode set
        if (32 <= n <= 126):
            result_string += chr(n)
    return result_string


def main():
    last_event = arrow.now('US/Eastern').replace(hours=-2)
    i = 0
    while True:
        # ackPL = [1]
        #radio.powerUp()
        now = arrow.now()
        if not radio.available(0):
            time.sleep(.01)
            #i += 1
            #print("{} - Radio not available".format(i), end="\r")
            continue
        receivedMessage = []
        radio.read(receivedMessage, radio.getDynamicPayloadSize())
        #print("Received: {}".format(receivedMessage))
        translated_string = translate_message(receivedMessage)
        if (now - last_event).seconds / 3600 > 1 and translated_string != "TEST":
            logger.debug('Sending MQTT message')
            (rc, mid) = client.publish("dog-monitor/food", now.ctime(), qos=1)
            last_event = now
        #(rc, mid) = client.publish("dog-monitor/food", translated_string, qos=1)
        logger.debug("{} - Received message: {}".format(now, translated_string))


if __name__ == "__main__":
    main()

