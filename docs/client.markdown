---
title: Beatled Controller
layout: default
nav_order: 4
---

# Beatled WebApp Client Controller

In order to control the system, users can access to a webapp via the browser. The app is build in React and is located in the `client` folder.

- To build the app, just run `npm run build`. (Note that this process is part of the Docker build and the app is pushed to the Raspberry Pi host together with the server.)

- You can access the webapp via `https://raspberrypi.local:8080/` (replace the domain name with your Raspberry Pi's local domain name).

- That the app is served over HTTPS in order to be able to be installed as a WebApp on iOS devices. By default we are using a self-signed certificate which will cause security warnings in browsers (and Safari on MacOS will refuse to load API endpoints).

![Image](/beatled/assets/images/client.png){: width="350" }
