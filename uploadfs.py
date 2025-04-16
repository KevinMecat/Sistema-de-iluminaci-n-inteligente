from platformio import util
from os.path import join, normpath

def before_upload_fs(*args, **kwargs):
    print("Generando el sistema de archivos SPIFFS...")
    util.exec_command("python -m platformio.tar -czf .pio/build/esp32dev/spiffs.bin data")

before_upload_fs()