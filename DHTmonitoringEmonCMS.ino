/* Used Libraries */
#include <ESP8266WiFi.h>       // ESP base library
#include <WiFiClient.h>        // ESP base library      
#include <ESP8266WebServer.h>  // ESP base library
#include <ESP8266HTTPClient.h> // ESP base library
#include <DHT.h>               // DHT sensor library
#include <SimpleThread.h>      // Millis() based library
/* Pins and DHT model */
#define DHT_DATA_PIN 4           // ~D4 (ESP8266-12)
#define DHTTYPE DHT11            // Model
DHT dht(DHT_DATA_PIN, DHTTYPE);  // DHT Start
/* Wi-Fi Config */
const char* ssid = "insertnetworkID";
const char* password = "insertpassword";
/* Webserver port: 80 */
ESP8266WebServer server(80);
/* EmonCMS Config */
String host = "emoncms.org";
String apikey = "yourAPIkey";
/* Variables */
const int intervalUploadServer = 10;     // Intervalo de update
String chipId = String(ESP.getChipId()); // Identify ESP by serial number
float humidity = 0;
float temperature = 0;
SimpleThread intervalUpdateServer(intervalUploadServer * 1000); //Objeto do SimpleThread
/* Reserved functions */
void sendServer();
void HTTPSendPost(String url);
/*******************************************************
  setup:
  Configures the Wi-Fi and the Serial Port
*******************************************************/
void setup()
{
  Serial.begin(115200);                    // Set baud rate
  WiFi.begin(ssid, password);
  Serial.print(" ");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  /* Logging status */
  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}
/*******************************************************
  loop:
  Executes the readings from time to time and sends data
  through HTTP to EmonCMS
*******************************************************/
void loop()
{
  /* Readings */
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  /* Waiting for readings */
  if (isnan(humidity) || isnan(temperature))
  {
    Serial.println("Error!");
    return;
  }
  sendServer(); // Function to send data to EmonCMS
}
/*******************************************************
  HTTPSendPost:
  Takes the url, constructed in sendServer, and post
  in the API
*******************************************************/
void HTTPSendPost(String url)
{
  HTTPClient httpPost;
  boolean postError = true;               // Error flag
  int contError = 0;                      // Counter of error of POST

  if ((WiFi.status() == WL_CONNECTED))
  {
    while ((postError) && (contError < 3))                      // 3-times verification, in case of post error
    {
      httpPost.begin("http://" + url);                          // Begin to post
      Serial.printf("[HTTP] POST (%i)...\n", (contError + 1));  // Shows No. of tries
      int httpCode = httpPost.GET();                            // Start HTTP
      if (httpCode == HTTP_CODE_OK)                             // Post if it finds "OK"
      {
        postError = false;                                      // Post OK
        String payload = httpPost.getString();                  // Gets payload
        Serial.println(payload);                                // Prints in serial
      }
      else
      {
        // If a post error occurs it increments No. of tries
        postError = true;
        contError ++;
        Serial.printf("[HTTP] POST... failed, error: %s\n", httpPost.errorToString(httpCode).c_str());
      }
      httpPost.end();                                           // Ends server connection
    }
  }
}
/*******************************************************
  sendServer:
  It sets an update time to send data to the server and
  it arranges the info into the JSON patterns.
*******************************************************/
void sendServer()
{
  if (intervalUpdateServer.check())      // Check elapsed time (default: 10s)
  {
    // String construction
    String urlSend = host;
    urlSend += "/input/post.json?json=";
    urlSend += "{";
    urlSend += "Temp_" + chipId + ":";   //Temp_ID
    urlSend += String(temperature);
    urlSend += ",";
    urlSend += "Humi_" + chipId + ":";   //Humi_ID
    urlSend += String(temperature);
    urlSend += "}";
    urlSend +=  "&apikey=" + apikey;
    HTTPSendPost(urlSend);
    // Serial logging
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" Humidity: ");
    Serial.println(humidity);
  }
}
