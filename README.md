# The Pump it Up Ticket Dispenser Document
## Written in 2022 by BatteryShark

## Preface
I can't believe I'm actually writing this - have I really scraped the bottom of the barrel wrt PIU that we're emulating the ticket dispenser? Fine - let's do this.


## Overview
I've never seen one of these IRL - from what I can tell, PIU Zero 1.03 was the first version to have code in the game to support it.
Essentially, the ticket dispenser is powered by an EZUSB controller with what I'm guessing is a pretty simplistic custom firmware.

## The Interface
Overall, the interface of the controller is pretty simplistic with 2 relevant bits for either read or write.

For input from the controller, we get a state bit representing the left and right dispenser and if it's currently available to accept a new command or... from what I understand "busy" which is likely only when it's printing a ticket or hanging.

This is represented in the code as "sensortbl" and lives on the sixth byte of our 8 byte array in bits 0x10 and 0x04. Due to its simplicity, we only have 4 real states that can be represented:

* 00000000 -- Both dispensers are idle.
* 00000100 -- Player 2 (Right) is dispensing, but Player 1 (Left) is idle.
* 00010000 -- Player 1 (Left) is dispensing, but Player 2 (Right) is idle.
* 00010100 -- Both dispensers are... dispensing.


From the other direction, the commands to the controller are represented in the code as "outtbl" and lives on the third byte of our 8 byte array in bits 1 and 2. Due to its simplicity, we also have only 4 states for this that can be represented:

* 00000000 -- No Request to Dispensers.
* 00000001 -- Request to Player 1 (Left) dispenser to dispense a ticket.
* 00000010 -- Request to Player 2 (Right) dispenser to dispense a ticket.
* 00000011 -- Request to both players' dispensers to dispense a ticket.


## Initialization
As such, windows (dev) builds will look for it via "\\\\.\\ezusb-0" -> "\\\\.\\ezusb-3" and Linux will search either "/prov/bus/usb/devices" or "/sys/kernel/debug/usb/devices" depending on how fucking... ancient the kernel is.

Either way, it's looking for a specific VID/PID combination (0D2F/1004) - where the Linux version will parse the text output of its devices list and Windows will use DeviceIOControl to request the device descriptor via "IOCTL_Ezusb_GET_DEVICE_DESCRIPTOR".

It will then try to open a persistent handle for that device ("usbfd").

Provided that works out, we run "TicketUpdateStatus" from our API and set the global "g_fTicketInitialized" so the game knows we have a ticket dispensing controller at the ready.

## API State
The game keeps track of the internal state of each dispenser in 4 different states:

* ticketIDLE: No dispensing requests - the default "resting" state.
* ticketSTARTREQ: A dispensing request is being made to a dispenser.
* ticketWAITSTART: A response from the controller is pending to acknowledge the request.
* ticketWAITEND: A response from the controller that the request is completed.
* ticketERROR: An irrecoverable error state.

It keeps track of the state in a local called "Stat".

In addition, it uses some locals (nReq, nOut) to determine how many tickets have been accumulated, and how many have been dispensed.


## API Calls
The following calls are used in the game or internal:

### TicketUpdateStatus
Usage: Internal Only
Pretty much, this just reads 8 byte state buffer from the usb controller. The states are outlined in "The Interface" above.

### TicketInit
Usage: in PIU's main function on game startup.
Does everything listed in initialization, takes two arguments representing the current ticket count to print for both dispensers - hardcoded to 0,0.

### TicketTerminate
USage: in PIU's main function on game shutdown.
Clears our locals (nReq,nOut,Stat = ticketIDLE) and then calls "TicketProcess"

### TicketStatus
Usage: GameOver State itself under flowcontrol, OR as part of the "PlayMovie" class when the state is set to play the "Game Over" video.
Determines if the dispensers have initialized, are in an error state, have no tickets, or have any tickets left to print. Returns value if pending print.

### TicketProcess
Usage: Timer Handler, called every tick after the IO/ButtonBoard refresh - our state check.
This does quite a bit. First, it checks if we've even initalized the dispenser controller and if not, short circuits.
It then calls TicketUpdateStatus to get the most up-to-date state information from the controller.

What it does next is highly dependent upon the current internal ticket state of both players (refer to "API State") for a description. In addition, the ticketWAITSTART and ticketWAITEND operate on a tick-based timeout of 200 attempts before erroring.

ticketIDLE: This will unset the output bit for this player.

ticketSTARTREQ: This will set the output bit for the player, requesting the dispenser to dispense a ticket. It then sets the internal state to "ticketWAITSTART" and resets an internal counter ("tick").

ticketWAITSTART: this will check if the output bit is set, signaling that the dispenser has acknowledged the request and is going to dispense the ticket. If it is set, the state is changed to ticketWAITEND and the internal counter ("tick") is reset. If not, it will increment "tick" and check if it has hit the CARD_TIME_OUT threshold, at which point it will set our state to ticketERROR.

ticketWAITEND: this will check if the output bit is unset, signaling that the dispenser is finished with the current request. If the output bit is still set, this means the dispenser is busy and will increment the "tick" value and clear requests/ticketERROR as the same with "ticketWAITSTART". If the output bit is unset, an internal total of current tickets to dispense is checked against a running total of dispensed tickets so far (nReq / nOut). If we have reached that value, requests are cleared for that player and the internal state is set back to ticketIDLE. If not, nOut is incremented and the state is set to ticketSTARTREQ to request another ticket be dispensed.

ticketERROR: At some point, an error state was hit. This is irrecoverable and simply clears each output request.


### TicketReset
Usage: GameOver state change
Resets the internal state of dispensers to ticketIDLE and wipes all current ticket counts. Dependent upon TicketStatus check not having any additional tickets to dispense.

### TicketRetry
Usage: GameOver state change and either center step is held down, and tickets are pending dispensing.
This essentially tries to reset the ticket state to ticketIDLE and calls TicketProcess again.

### TicketAdd
Usage: None
This increments the amount of tickets given a player. Probably for testing purposes and is now unused.

### TicketSet
Usage: Title screen, station select, usb in (nx2+), song select when a player joins, also on SetTicketOutput
Sets an amount of tickets to an arbitrary number.
For joining, it will always set your ticket to 0 or 1 depending on if the MercyTicket option is enabled.
See "SetTicketOutput" for details on post-state setting.

### SetTicketOutput
Usage: NextStage and StageBreak
Depending on which players are enabled, this will calculate the current score and determine if tickets need to be added. There is a machine option for 1 ticket every 100,000 points and this looks to be hardcoded in most cases although I think some games have an option to have another score... it may not work. Also it will take into account the mercyticket again at this point too.

What's probably a bit more scary is that gs_100kPointPerTicket defaults to 0 and the calculation is as follows:
g_Option.gs_MercyTicket + (g_Player[0].m_dwTOTAL_SCORE / 100000 / g_Option.gs_100kPointPerTicket);


### DisplayTicketInfo
Usage: Draw call update loop
Shows some output debug info to the screen about current/pending tickets, state, etc.
If debug isn't enabled, this will also show the ticket dispenser error or "No Ticket" message if needed.


## Controller Emulation
Emulating the controller in software then becomes pretty straightforward:
- On a Read from the controller we:
	Check the last written state from the game.
	If P(n) has a new request, and our internal state is clear to accept, we set the output bit.
		If there is no request, or our internal state is busy, we don't set the output bit.

## Additional Notes
It seems like Zero might be buggy and always print for both dispensers. It's possible that this build was not fully finished
to support tickets as the settings are null or unset and starting with P2 errors the dispenser automatically. This likely works fine
on newer builds, however.

In addition, the specification originally included a bit to denote when a ticket dispenser was empty... it is unused and might have been for another game as #define FOR_PIU defines are found in various places and may indicate some reuse.

Also, it appears that the mercy ticket prints immediately when hitting center step and then each ticket prints after a song if you hit 100k points. There is no end-of-credit mass print of tickets as I had originally expected.

Also, if the ticket dispenser is erroring, the gameover screen will hang and allow you to infinitely retry the dispenser by hitting center step and will not clear on its own unless the dispenser clears itself.

For better emulation, this really should have a separate thread doing the ticket processing - among other QOL changes. An exercise for someone else, I suppose. 

