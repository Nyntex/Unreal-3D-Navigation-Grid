Heyho, in this Project you find 2 C++ Classes and 2 Blueprint Classes + 1 level to showcase what I've done.
The C++ classes include the 3D Navigation Grid that can be added to any level and with the Acotr Component you are able to give commands like getting a new path or move to position.
I did not include latent actions like the origianl Unreal Character Component "Move To" functions as I didn't work with those yet and would first need to inform myself.

--------------------------------------------
How it works
--------------------------------------------

But how do they move if they are not using latent actions?
Quite a simple and cheesey workaround! As I already said I also added a new Actor Component, this component just moves with the Tick function giving the actor movement inputs into the direction of the next point that this character should move to.

But how do I know when it is finished moving?
Also quite simple: Once it finishes moving I invoke "Finished Moving". Any class that needs to know if the character finished moving can bind to this event to receive the event call. 

How does the pathfinding work?
I'm using an A* algorithm on a three-dimensional array of nodes. The Node struct can also be found in the c++ code. So I get the start and end node I want to move to based on the positions I give as parameters. 
You can either give Actors or Vectors as input paramaters while actors are being prioritized over the vector. Once called I search for the nodes I want to move to and calculate their cost based on the distance to the goal node and the total moves that you need to do to get to that node
The next node that gets checked will always be the one that has the lowest total cost, this way I will find one of the fastest ways to get to the desired destination

-------------------------------------------
End Note
-------------------------------------------

This whole thing is not completely finished, but it's working as you'd expect. In the future I plan to add latent move actions instead of this hacky workaround and general improvements I don't know yet.
