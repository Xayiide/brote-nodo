# Nodo para el proyecto brote

# Instrucciones

Es necesario tener instalado el entorno ESP8266-RTOS-SDK con las instrucciones
de su [página web](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/linux-setup.html).

Para utilizar Eclipse, se tendrá que seguir también sus [instrucciones](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/eclipse-setup.html) y, además, añadir lo siguiente en `C/C++ Build > Environment`:
1. Variable `VIRTUAL_ENV` con el valor `/home/user/esp/.venv` o la ruta a donde sea que tiene esp-idf el entorno virtual
2. Variable `PATH`, además de la ruta al ESP8266-RTOS-SDK, añadirle también `/home/user/esp/.venv/bin` o la ruta al entorno virtual.

# credentials.mk

En el directorio raíz tiene que haber un fichero `credentials.mk` donde se definan los siguientes símbolos:
- `WIFI_SSID`: SSID de la WIFI a la que se conectará el nodo.
- `WIFI_PASS`: Contraseña de dicha WIFI.
- `DST_IP`: IP destino a la que enviar datos y logs.
- `DATA_PORT`: Puerto al que se enviarán los datos.
- `LOG_PORT`: Puerto al que se enviarán los logs.
