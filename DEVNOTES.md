## Adding a new config variable

### Backend

* src/brineomatic_config.h -> add default #define if needed
* controller.h / channel.h / brineomatic.h
    * add new variable with default
* controller.cpp / channel.cpp / brineomatic.cpp
    * add var to generateConfigJSON()
    * add var to validateHardwareConfigJSON() or equivalent
    * update loadHardwareConfigJSON() or equivalent
* add custom code and logic for use of new variable

### Frontend

* js/Brineomatic.js or js/channels/channel.js
    * add html to generateEditUI()
    * update editUIData()
    * update UI visibility (if needed)
    * update getConfigFormData()
* add custom code and logic for use of new variable