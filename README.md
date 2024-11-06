# 3D Navigation Grid for Unreal Engine

This is my first project made during my 3rd semester at [S4G school for Games](https://www.school4games.net/). I created this for our 3rd semester project for flying enemies which sadly didn't make the cut.
But that doesn't stop me from putting it in here and showcasing my work.



## Grid

The grid is a three dimensional array of nodes that get created and validated on start. I wanted to save them as an editable variable but Unreal really struggles with showing arrays in the editor, dropping fps to 5 or less when showing 1.000 or more items, which is unfortunate at best. Therefore the array gets filled on start and after extensive testing I figured it would be no problem. 

The grid visualized when generating, showing which nodes collide with walls and showing nodes connecting to their neighbors:  
<img src="https://github.com/user-attachments/assets/b80d10b7-c40d-4cc7-bc45-f7e1c8750b64" width="400">


And which nodes are actually usable nodes (green) and which are blocked (red):  
<img src="https://github.com/user-attachments/assets/86039be3-c165-4651-b975-e255018d0ccc" width="400">


## Latent Action

With this grid we can use a new node, the "Move to Actor or Location 3D"-Node which handles the movement of the APawn calling it. However, this only works if the APawn uses [Unreal Engine's Character Movement Component](https://dev.epicgames.com/documentation/en-us/unreal-engine/networking-and-multiplayer-in-unreal-engine?application_version=5.4), because I use the movement input. I know not everybody would use this component and I'm certain there are better ways but most of them will include creating either a new movement component or having a huge amount more code.  
This node can only be called once per APawn, canceling any other node of this type resulting in the cancel node being called. Also, if you use this node make sure there are no other Async Movement Nodes running as they interfere with each other.  

![image](https://github.com/user-attachments/assets/3a078c3e-bfd2-45e8-aacb-341707d4cef0)

It can be used to make constant movement, like this:  
![image](https://github.com/user-attachments/assets/c74777cd-8b03-4ae9-a045-592cf1c64c90)

## (Old) Movement Component

This repo does also have a movement component which has to be used alongside Unreal's Character Movement component, because, just like the node before, it uses the movement input. This component is made completely in blueprints 

<img src="readme/NavigationTester.png" width="700">

<img src="readme/MovementComponent.gif" width="400">

---

## End Note

This project is not yet finished, there are still some edges I want to smooth out. I want to add an async gameplay action next to the latent action and want to make sure that there are more inputs and outputs to have anything that one might need available. Also, there are still some bugs to fix and features to add, like dynamic path evaluation

---

This project was made using the [Unreal Engine 5.4](https://www.unrealengine.com/en-US/) by Epic Games
