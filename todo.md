# First Level Heading

KDE Plugin Asks

- [ ] API to wake the phone.  (Current code in DeviceStatus::ping calls forceOnNetworkChange.
- [ ] Add a request-identifier to requestAllConversations and requestConversation.
  Replies should include the request identifier
- [ ] Replies to requestConversation should include the index of the message
- [ ] requestAllConversations should return the number of threads it's going to send
  or otherwise indicate when it's reached the end of a request.

## Status
- [ ] Better state icons
- [ ] Reset KDE button

## Message List
- [x] Date calculation not done until the 30 second tick
- [x] Date not centered
- [x] From-Me blocks are not right aligned
- [x] all dates should have day-of-week
- [x] Colors should be more striking - incoming color is okay, outgoing should be white-on-dark-blue
- [ ] Group Chat should show senders
- [ ] Seeing cases where the time is "28565m"
- [x] Chat header should have an avatar and put the date on its own
- [x] Convo Header should have participant(s) icon then comma-separated list of participants
      - [x] Show avatar in conversation heading
      - [x] De-dupe the partcipants list
- [x] Implement sending messages from conversation
      - [x] Minimal send - just send and hope for the best
      - [x] Keep message in pending state until it shows up in the conversation
            (Any new message that's from the user will clear this state)
      - [x] Message area should be saved when switching conversations with an unsent text
      - [x] Animate button appearing/disappearing
- [ ] Attachments
      - [ ] Blank messages with just attachments show as 'Image' in headers
      - [x] Enable displaying the download links -- do this for all mime types.
      - [x] Enable displaying images
      - [x] Enable drag&drop files into the send-message
      - [x] Enable cut&paste images
      - [x] Initialize state from cache correctly in setAttachments
      - [ ] Fix issue where animated gifs loaded late don't animate sometimes
      - [x] MakeMessagesHandler should have a 'checkForCachedItemsInQueue' method instead of the
            thing that we have now that just looks at stuff at the head.
      - [x] Put mutexes around attachment queue access
      - [ ] Consider making all images smaller and having a hover-over enlarge button in the upper right
            and the 'open' button can do that too.

## Headings
- [ ] Trim newlines on messages
- [x] Make avatars? for participants
- [ ] Fix selection animation
- [ ] Headings should show an indicator that there's a draft message

## Main Window
- [ ] Splitter should allow maximizing one side or the other
- [ ] Fix Title (away from Hello World)

## Test infrastructure
- [x] Implement simulated message arrival
- [ ] Incremental load of messages
- [ ] Full load of messages
  - [ ] Check multi-target messaging
`


