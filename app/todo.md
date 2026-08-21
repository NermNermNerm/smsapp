# First Level Heading

## General
- [x] Write a readme
- [x] self-install, after done run:  execve(argv[0], argv, environ);
- [x] copy 2FA codes to the clipboard
- [ ] On initial startup, it doesn't show the conversations, only a red dot, but later recovers itself.
      Repro: Delete ~/.config/NermNermNerm
- [ ] Start-menu-icon.png is stored in ~/.local/share/appsmsapp/ and cache data is stored
      in ~/.local/share/NermNermNerm/SmsApp.
- [ ] Delete cache data from devices that are no longer there
- [ ] The KDE Deamon doesn't update itself after coming back from sleep; we should force that
- [ ] When you halt the fake kde daemon, the status goes red but it doesn't blank the screen...  Is that bad?
      At the least there should be a tooltip over the red dot.
- [ ] Fix issue where animated gifs loaded late don't animate sometimes
- [ ] Add attachments with Ctrl+O
- [ ] Add a phone locator button...  But where?
- [ ] Emoji menu
- [x] Upload it all to github.

## Search
- [ ] Need to be able to search contacts and threads.
- [ ] Add contacts to conversations
- [ ] Start new threads

## Pin to start
- [ ] Make the test fixture able to do devices that don't support sms (And make list show id's while you're at it.)
- [x] Add a pin-to-start button -- note that pin-to-start should specify --device iff # of sms devices > 1
- [x] Add an "--autostart" flag; If --autostart flag exists, start minimized.
- [x] If invoked with --autostart and KDE says the device doesn't exist and we're sure that KDE is booted, exit and delete the autostart entry
- [x] If --device and there's only one sms-capable device, delete the --device from the autostart entry.
- [x] If not --device and there's more than one sms-capable device, add the --device flag to the autostart.

## UX
- [ ] Let user choose phone icon color
- [ ] Dark-mode
- [ ] UI Scale
- [ ] Localization
- [ ] Background that matches phone icon color


## Headings
- [x] Trim newlines on messages
- [x] If draft message, it should be in italics like "You: blah blah"
- [x] should show whether last message is to or from you
- [x] Blank messages with just attachments show as 'Image' in headers
- [x] Highlight unread threads
- [x] Fix selection animation
- [x] When phone starts unreachable, it doesn't show any cached data
- [x] Make avatars? for participants
- [x] Changing the message handler should wipe the Drafts dictionary
- [x] Just-arrived messages say '0m' and there should be a 30-second timer to redraw dates.
- [?] When connecting to the live phone and i knew there was a message on the 2nd thread (per the old ordering), it didn't download the fresh message until I clicked on the heading.  It could be the top-two threads were undownloaded, 

## Status bar/Tray
- [x] QML/Settings warnings on launch
- [x] Show an indicator when new messages are there
- [x] Tooltip should show whether there are messages or not
- [x] Clicking on the tray icon should open the app
- [x] Multiphone
  - [x] Use different background colors for different specified phones

## Main Window
- [x] Show a yellow bar when the phone is unreachable, warning the user that it's not up-to-date
- [x] Show battery state instead of green dot for phone connected
- [x] When no phone is reachable and there is no preferred device, it should create a message that takes over the screen
      telling the user to pair a device.
- [x] When KDE Appears to be down, and no phone is configured, it should take over the screen with a message telling
      the user to install kde
- [x] Multiphone
  - [x] Show other phone launcher buttons
  - [x] Use different main window background colors per phone too
- [x] Splitter should be banned from maximizing to one side or the other and should remember its setting
- [x] Fix Title (away from Hello World)
- [x] Remember window size and position

## Message List
- [x] Can't copy/paste from messages
- [x] When the phone is unreachable, the send button should be disabled
- [x] Date calculation not done until the 30 second tick
- [x] Date not centered
- [x] From-Me blocks are not right aligned
- [x] all dates should have day-of-week
- [x] Colors should be more striking - incoming color is okay, outgoing should be white-on-dark-blue
- [x] Seeing cases where the time is "28565m"
- [x] Chat header should have an avatar and put the date on its own
- [x] Convo Header should have participant(s) icon then comma-separated list of participants
      - [x] Show avatar in conversation heading
      - [x] De-dupe the partcipants list
- [x] hyperlinks
- [x] Group Chat should show senders
- [x] Implement sending messages from conversation
      - [x] Minimal send - just send and hope for the best
      - [x] Keep message in pending state until it shows up in the conversation
            (Any new message that's from the user will clear this state)
      - [x] Message area should be saved when switching conversations with an unsent text
      - [x] Animate button appearing/disappearing
      - [x] Send attachments with drag&drop
            - [x] Figure out why the attachments aren't sending
            - [x] Need to disable drag&drop while message is sending
            - [x] Need to clear area after message is sent
            - [x] DropArea should encompass whole messages control
            - [x] Send button should be there when there's an attachment
            - [x] Return key processor still uses getAll()
            - [x] Phone seems to be silently failing to process texts with attachments
               - [x] Limit total size to 600mb
      - [x] Send attachments with paste
      - [?] Kvetching on the console when pasting non-images
      - [x] Outgoing images don't shrink correctly when they are wide - they clip
      - [x] Sending area is popping down even though the message got eaten.
            - [x] Add a message bar that pops up after the message looks doomed allowing cancel
      - [-] Add a file-open dialog and a ctrl+o handler
      - [-] look into a send-to-phone capability
- [x] Display Attachments
      - [x] Enable displaying the download links -- do this for all mime types.
      - [x] Enable displaying images
      - [x] Enable drag&drop files into the send-message
      - [x] Enable cut&paste images
      - [x] Initialize state from cache correctly in setAttachments
      - [x] MakeMessagesHandler should have a 'checkForCachedItemsInQueue' method instead of the
            thing that we have now that just looks at stuff at the head.
      - [x] Put mutexes around attachment queue access
      - [x] Consider making all images smaller and having a hover-over enlarge button in the upper right
            and the 'open' button can do that too.

## Test infrastructure
- [x] Implement simulated message arrival
- [x] Incremental load of messages
- [x] Full load of messages
- [x] Check multi-target messaging
- [x] Nice up the command parser and add help.
- [x] Multiphone
      - [x] Add commands to create a new device

## KDE Plugin Asks

- [ ] API to wake the phone.  (Current code in DeviceStatus::ping calls forceOnNetworkChange.
- [ ] Add a request-identifier to requestAllConversations and requestConversation.
  Replies should include the request identifier
- [ ] Replies to requestConversation should include the index of the message
- [ ] requestAllConversations should return the number of threads it's going to send
  or otherwise indicate when it's reached the end of a request.


## Compilation requirements

### Kpeople_lookup
- ECM -- sudo zypper install extra-cmake-modules
- KF5Contacts
  * WRONG: sudo zypper install libKF5Contacts-devel
  * SKETCHY: sudo zypper install kf6-kcontacts-devel
  - Qt5:
    * maybe: sudo zypper install libqt5-qtbase-devel

### Main app
- sudo zypper install qt6-svg-devel
- sudo zypper install qt6-multimedia-devel
- sudo zypper in libphonenumber-devel






