# First Level Heading

- [x] Upload it all to github.
- [ ] Write a readme

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
- [ ] copy 2FA codes to the clipboard
- [ ] Multiphone
  - [ ] Use different background colors for different specified phones

## Main Window
- [x] Show a yellow bar when the phone is unreachable, warning the user that it's not up-to-date
- [x] Show battery state instead of green dot for phone connected
- [ ] Add a pin-to-start option
- [x] When no phone is reachable and there is no preferred device, it should create a message that takes over the screen
      telling the user to pair a device.
- [x] When KDE Appears to be down, and no phone is configured, it should take over the screen with a message telling
      the user to install kde
- [ ] Multiphone
  - [ ] Show other phone launcher buttons
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
      - [ ] Fix issue where animated gifs loaded late don't animate sometimes
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
- [ ] Multiphone
      - [ ] Add commands to create a new device
`


## KDE Plugin Asks

- [ ] API to wake the phone.  (Current code in DeviceStatus::ping calls forceOnNetworkChange.
- [ ] Add a request-identifier to requestAllConversations and requestConversation.
  Replies should include the request identifier
- [ ] Replies to requestConversation should include the index of the message
- [ ] requestAllConversations should return the number of threads it's going to send
  or otherwise indicate when it's reached the end of a request.

