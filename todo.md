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
- [ ] Date not centered
- [ ] From-Me blocks are not right aligned
- [ ] Trim trailing newlines
- [ ] This-week dates should be "tuesday"
- [ ] all dates should have day-of-week
- [ ] Colors should be more striking - incoming color is okay, outgoing should be white-on-dark-blue
- [ ] Group Chat should show senders
- [ ] Chat header
- [ ] Convo Header should have participant(s) icon then comma-separated list of participants
  (For group chats, the group gets an icon built for it)
- [ ] Implement sending messages

## Headings
- [ ] Trim newlines on messages
- [ ] Make avatars? for participants
- [ ] Fix selection animation

## Main Window
- [ ] Splitter should allow maximizing one side or the other
- [ ] Fix Title (away from Hello World)

## Test infrastructure
- [ ] Implement simulated message arrival
- [ ] Incremental load of messages
`


