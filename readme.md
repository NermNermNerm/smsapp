# Linux SMS App

This is an app is basically a replacement for the stock KDE Connect SMS app.  At this point,
it presents a much nicer UI, but struggles to reliably show messages due to the limitations
of the underlying KDE Connect SMS plugin.

The short-term goal is to get something that works with the existing kde sms plugin and
offers a better experience than the stock app.  As of this writing, I think we're there.
In addition to just looking better, it has details like convenient pin-to-start and automatically
copying 2FA codes to the clipboard.

However, the "better than the stock app" isn't a high bar.  The reliability deficiencies
make that app frustrating.  It's good when it works, but the fact that it so often doesn't
makes it of dubious value.

In the medium term, I feel like what we really need to do is just straight up rewrite
the sms api.  At a minimum, the new api should offer:

1) A way to wake the phone if it's on wifi.
2) Take/skip searches.
3) A clear indication of what signals are in response to what request.
4) Reporting of errors and progress.
5) The new API should not have its own cache or at least make it so
   the caching is invisible to the callers.

In the long term, what I really would like is for this thing to do everything that the
Microsoft and Apple phone integration apps do, including telephony.

## Building and running

My desktop is Linux Mint Cinnamon with Qt Creator 20.0.0.  Beyond that I honestly have
no idea what I've had to install to get it to build.  Your best bet is to build the app from
Qt Creator 20, but there's a chance it'll build for you if you do the usual:

```sh
cd app
mkdir build
cd build
cmake ..
make
```

You'll also need to build the 'kpeople_lookup' subdirectory.  That one, because dependency hell,
cannot be built from Qt Creator and must be built using command-line cmake.

If you're serious about wrenching on it, what you want to do is also build fake-kdeconnect
(from Qt Creator or command line).  It's a dbus service that mimics KDE Connect for all the
api's that the app uses.  It doesn't replace kde-connect, it registers itself at 'org.fake.kdeconnect'.
When you start the app, it tries to connect to that address and actually falls back to the
real kde connect if the fake one doesn't respond.

It has a rough&ready command-line interface as well as logs the dbus activity.  The first
command you'll want to run is 'kdelistdevices' which just talks to the real kde connect
and gets a list of device id's.  Next, run 'kdepopulate &lt;deviceid&gt;'.  What that'll do is
read all your text messages into a json file so you've got a fake dataset to start with.
All the data this thing uses is stored in a hard-coded path, ~/fakekde.  That download will take
some time, so be prepared to go get a coffee or something.

You can do stuff like simulate the device going unreachable with "on" and "off".  You can
simulate incoming text messages with commands like "text sarah Hey did you fix that stupid app yet?"
