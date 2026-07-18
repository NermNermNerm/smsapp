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
- [ ] When phone starts unreachable, it doesn't try to nudge it
- [ ] Settings:
    qt.qml.propertyCache.append: Member implicitHeight of the object QQuickItem_QML_53 overrides a member of the base object. Consider renaming it or adding final or override specifier
    qrc:/qt/qml/smsapp/Main.qml:14:5: QML Settings: Failed to initialize QSettings instance. Status code is: 1
    qrc:/qt/qml/smsapp/Main.qml:14:5: QML Settings: The following application identifiers have not been set: QList("organizationName", "organizationDomain")
    qrc:/qt/qml/smsapp/Main.qml:14:5: QML Settings: The Settings type from Qt.labs.settings is deprecated and will be removed in a future release. Please use the one from QtCore instead.

"Max message size limit exceeded."

## Headings
- [ ] Trim newlines on messages
- [x] Make avatars? for participants
- [ ] Fix selection animation
- [ ] Headings should show an indicator that there's a draft message
- [ ] When phone starts unreachable, it doesn't show any cached data
- [ ] Blank messages with just attachments show as 'Image' in headers
- [ ] should show whether last message is to or from you
- [ ] threads with last-message-transmitted should look distinct from received ones
- [ ] remember a last-read date per thread

## Message List
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
- [ ] Group Chat should show senders
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
      - [ ] Add a file-open dialog and a ctrl+o handler
      - [ ] look into a send-to-phone capability
- [ ] Display Attachments
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


## Main Window
- [ ] Splitter should allow maximizing one side or the other
- [ ] Fix Title (away from Hello World)
- [x] Remember window size and position
- [ ] Status bar should hide itself when phone status is green
      (be visible for ~3 seconds after the window becomes visible or status becomes green)
- [ ] copy 2FA codes to the clipboard

## Test infrastructure
- [x] Implement simulated message arrival
- [x] Incremental load of messages
- [x] Full load of messages
  - [x] Check multi-target messaging
`


