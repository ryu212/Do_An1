run the script 'start_server.sh' turn on wifi hotspot and mossquitto MQTT server, then run the Python file '/Python/visualize.py'  to visualize the data flow.



During the visualization, do not forget to connect the ESP32 to public data to MQTT server



stm32 Master-slave esp32



esp | STM | function
-----------------------
P23 | PA7 | MOSI
P19 | PA6 | MISO
P18 | PA5 | Sclk
P5  | PB0 | CS
    | PA0 | ADC
