idf_component_register(
    SRCS 
    "Application/Data.c" 
    "Network/Mqtt.c" 
    "Network/Wifi.c" 
    "main.c" 
    "Driver/Spi.c"
    "Driver/SDCard.c"
    INCLUDE_DIRS 
        "."
        "Driver"
        "Network"
        "Security "
        "Application"
    EMBED_TXTFILES
        "Security/CA.pem")