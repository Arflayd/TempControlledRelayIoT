# TempControlledRelayIoT
Arduino ESP32 based controller filling a role of a thermostat. <br>
Uses DHT11 Temperature/humidity sensor, SSD1306 OLED display, and a generic relay. <br>

It was made for a car heating system, which can be remotely switched on <br>
and will shut itself off when the specified temperature is reached <br>
(or after some time - as a safety feature). <br>

It uses SinricPro for easy access from an application, integration with Google Home and readings history. <br>

For it to work, simply connect the DHT11 sensor and relay control to desired pins, <br>
specify constants according to needs, provide the credentials for WiFi & SinricPro <br>
and provide 3.3v power to all the components. <br>



<table padding="0">
  <tr>
    <td><img src="https://user-images.githubusercontent.com/37042650/209230531-11f6dade-58b4-42f5-964e-9ec4e887f686.jpg" width="350"></td>
    <td rowspan="2"><img src="https://user-images.githubusercontent.com/37042650/209230537-0aacc7a0-5fe6-4667-acdf-56901947e050.jpg" width="250"></td>
  </tr>
  <tr>
    <td><img src="https://user-images.githubusercontent.com/37042650/209230526-1152a260-6f28-4419-b1ea-d463950cfa80.jpg" width="350"></td>
  </tr>
</table>


TODOs:
- Make the target temperature and timers configurable without reprogramming
- Use tasks and threading
- Explore solutions other than SinricPro, as it only allows for up to 3 device connections in the free tier



