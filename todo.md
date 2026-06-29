# First Level Heading

KDE Plugin Asks

- API to wake the phone.  (Current code in DeviceStatus::ping calls forceOnNetworkChange.
- Add a request-identifier to requestAllConversations and requestConversation.
  Replies should include the request identifier
- Replies to requestConversation should include the index of the message
- requestAllConversations should return the number of threads it's going to send
  or otherwise indicate when it's reached the end of a request.

## Bugs
- It looks like when a new message count comes along and
  overrides the cache, it reorders the conversation hash incorrectly
- When it requests a missing conversation message item,
  it never shows up.

## Get the status nice
- Better state icons

## Get Heading list nice
- Put the date to the right

## Get Messages working
- Write the code to manage the list of messages.
- Attachments
- Send

## Test infrastructure
- Full implementation of used API's