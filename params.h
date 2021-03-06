
#define TITLE          "IoT TH sensor"
#define SOFTWARE_NAME  "IoT TH sensor"

#define LOCATION_NAME  "Nappali"
#define IOT_DEVICE_ID   1


// =============================
// Pinout configuration
// =============================
 
#define DHT11_PIN                         14 // 14 means D5 in NodeMCU board -> Pin for temperature and humidity sensor


// Server functions
#define SERVER_PORT                       80

// connect to WiFi
#define WIFI_TRY_COUNT                    50
#define WIFI_WAIT_IN_MS                 1000

// delay for normal working
#define DELAY_BETWEEN_ITERATIONS_IN_MS 30000

// problem handling
#define ERROR_COUNT_BEFORE_RESTART         3
