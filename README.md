Heyho, in this Project you find 2 C++ Classes and 2 Blueprint Classes + 1 level to showcase what I've done.
The C++ classes include the 3D Navigation Grid that can be added to any level and with the Acotr Component you are able to give commands like getting a new path or move to position.
I did not include latent actions like the origianl Unreal Character Component "Move To" functions as I didn't work with those yet and would first need to inform myself.



But how do they move if they are not using latent actions?

Quite a simple and cheesey workaround! As I already said I also added a new Actor Component, this component just moves with the Tick function giving the actor movement inputs into the direction of the next point that this character should move to.

But how do I know when it is finished moving?
Also quite simple. Once it finishes moving I invoke "Finished Moving". Any class that needs to know if the character finished moving can bind to this event to know when it finished moving. 