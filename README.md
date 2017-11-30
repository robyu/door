DOOR
----
I wanted my phone to notify me when my garage door is open, so I built Door: an internet-enabled door sensor. 

The software runs on an ESP8266 (NodeMCU v2).

The sensors are magnetic reed switches.

When the door opens, a reed switch also opens. The Door software then sends a [webhook](https://ifttt.com/maker_webhooks) event over wifi to IFTTT, and IFTTT sends notification to

NOTES
-----
upload.sh is a script to build and load the software onto the ESP8266. You'll need to first install [platform.io](http://platformio.org/).

upload.sh expects there to be a file called webapi_url.txt, which specifies the webhook
URL.  The file should contain a single line of the form:
      \/trigger\/your_event_name\/with\/key\/key_string

replacing
	your_event_name with your webhook event name
	key_string with the webhook key

License
=======
MIT License

