# Unitscounter
To download the librairies, pgadmin 4 ver 7.3 and Wampserver  needed for this project visit : https://drive.google.com/drive/folders/1StSirbKY7WPqVwoXQzguuRQoSada5r45?usp=sharing

the final version of the code in FINAL CODE.ino

the capteurs.php file is for showing the postgresql's table with the localhost using Wampserver, it should be in this directory :  C:\wamp64\www 

Remark : the esp32 has a 4MB flash memory, so the number of lines considering an average of, say, 80 characters for each line, is up to 50000. that means the date and time of up to 50000 passing units can be stored in case the Wifi or the MQTT broker's connection broke down .
