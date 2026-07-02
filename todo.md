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




Okay, what we've been talking about so far is the conversation headers.  Let's think about the full conversation list now and (for a moment) just talk about that problem in isolation.
 - we'll initially show what's in the cache
 - we won't submit requests for more messages until the thread leaves the 'unknown' state.
   - bonus: the ui should display that it's holding off in this case and offer the user the option to 'forget' the thread
 - When we do submit a request, should we ask for the full list or limit it to just what's on the screen?  Once we send the request, it'll just keep piping them to us, but it's only the dbus' bandwidth we're up against, right?
 - When the user switches to another thread (that's not unknown), I think we have to just send off another request.  The only throttling I can figure we can do to limit the spam is to not actually send the request for more messages until some time has passed since the user landed on the page and/or the previous request was sent.
 - The cache can't do much for us other than initial population.

We need the same event handlers to deal with 




variables:
  conversationsNeedingUpdate: A list of thread id's.  Initially empty.
  knownThreads: set<threadid>


Upon startup, all threads that are in cache are flagged as 'unknown'.
We launch requestAllConversations

In conversationLoaded
  If the 'count' is zero:
    delete it from the cache and send a message to the UI to delete it.
  else if knownThreads doesn't contain the thread
    if There's a cached copy older than stableThreadTime:
      add the thread to knownThreads
    else
      add this thread to 'conversationsNeedingUpdate'  (assert it's not already there)
      initiate the request if there's not one outstanding

In ConversationUpdated
  if the message is for the head of the 'conversationsNeedingUpdate' list:
    if the message is identical to one in cache:
      set stableThreadTime = the time of this thread
    else
      emit a message to the UI about the updated thread

    pop the head of conversationsNeedingUpdate
    while the head of conversationNeedingUpdate is the id of a thread in the cache whose youngest timestamp is older than stableThreadTime
      Add the threadid of the head of conversationsNeedingUpdate to knownThreads
      pop the head of conversationNeedingUpdate
    if conversationNeedingUpdate is not empty
      requestConversation on the new head
  fi
  else // it must be either one we asked for in the list that shows all the messages for a thread or is new data
    if the message is identical to one in cache:
      do nothing
    else
      update cache
      emit a message to the UI about the updated thread
  add the thread to knownThreads

OLD In conversationLoaded
  If the thread is in the 'unknown' state and 'grabbingNewestThreads' is true
    add this thread to 'conversationsNeedingUpdate'  (assert it's not already there)
    and initiate the request if there's not one outstanding.
  If the 'count' is zero, delete it from the cache and send a message to the UI to delete it.
  ** Thought we needed, but actually not:  If the thread is not in the 'unknown' state, add this thread to 'conversationsNeedingUpdate' if it's not already there
  ** return immediately if previous message was conversationCreated and it referenced this threadId (we have nothing to gain)
     ^-- not needed either.  conversationCreated should mark the thread as 'known'.

OLD In conversationUpdated
  if grabbingNewestThreads and the message is for the head of the 'conversationsNeedingUpdate' list:
    if the message is identical to one in cache:
      set grabbingNewestThreads to false
      remove any threads in 'conversationsNeedingUpdate' that are flagged as 'unknown'
    else
      pop the head of conversationsNeedingUpdate
      if the list still has stuff in it
        requestConversation on the new head
      emit a message to the UI about the updated thread
    fi
  else // it must be either one we asked for in the list that shows all the messages for a thread or is new data
    if the message is identical to one in cache:
      do nothing
    else
      update cache
      emit a message to the UI about the updated thread

In conversationCreated
  Update the thread
  emit a message to the UI about the updated thread
  set a member so that 'conversationUpdated' can know that this message came in.
  set the state of the thread to 'knownExists'
  