# cube-game
Implementation of the simplest elements of minecraft in OpenGL.

## Overview
*(As of May 2021.)*

[Demo video here.](https://drive.google.com/file/d/1anIOkCU6k5jPAZ_KL5CpkQRP48T3tQ5s/view?usp=sharing)

### Project Description 
My initial goals for this project were to begin by generating a flat world of dirt blocks with stone blocks underneath until they reach some depth.  From there, I would use the same FPS camera functionality from Project 4 to allow the player to walk around the world, jump, and place dirt blocks using right-click. From there, I was hoping for the player to be able to break blocks using left-click, and then pick up those blocks.  I was also aiming to add more variety to the landscape by generating trees, various ores at different depths,  and varying the heights of the surface-level landscape rather than just having it be open and flat.  
I ended up implementing almost all of these functions in some form! The only one I didn’t quite get around to was incorporating blocks to drop and be picked up after they were broken.  In place of this, I made the block the player was placing the same as the last one broken, with a few caveats.  For instance, similar to the actual game, when you break a stone block, the player then can place cobblestone blocks.  

The most initially observable bug is that the side-texture of the grass blocks are upside for two of the four side faces.  I believe the easiest solution to this would be to either upload a second grass texture with a reverse orientation and use that texture when rendering, or to rotate the cube face before drawing it to the screen.  However, I did block texturing rather early on in the process and ultimately decided to pursue implementation of the more significant gameplay aspects rather than play around with the textures for too long.  Behind the scenes, the main obstacles I struggled with were player collisions and using the player’s camera to calculate which block they were looking at (to determine where to place or break blocks).  The player collisions still experience some significant bugs.  For instance, players can jump up the side of walls by moving towards that grid-space while jumping, where the algorithm improperly sees that given their next position, there is a block underneath them.   I also still need to touch up a player’s ability to walk smoothly touching walls without briefly sticking at the boundaries between grid spaces.  The final area for improvement is to not allow players to walk diagonally through grid spaces when they’re blocked on the other sides.  I talk more about how I selected what block the player was looking at below, but currently it is only limited to interaction with the top face of the block.  Outside of these two main mechanics, I don’t think I faced any major setbacks.  

### Future Work
While there are many obvious ways this project could be extended, I think what I personally would be most invested in initially would be to implement blocks dropping when they're broken, and then allowing the player to pick up those blocks. This would include ores dropping when they were mined.  I think another way to implement the basics of crafting and smelting would be to let the player right click on either a crafting table or smelting block and then transform the item in their hand (if applicable).  For instance, right clicking on a furnace with iron ore would result in an iron ingot, and right clicking on a crafting table with a log would result in wood planks.  I’d also like to improve the collision detection mechanics I discussed in the first section, as well as allow the player to interact with the bottoms and sides of blocks, rather than only the top. With all these areas for further functionality mentioned - perhaps the most obvious aesthetic improvement given the games current mechanics would be to create an animation for block breaking.

Outside of game functionality, I’d really like to improve how some of the programming is done.  I’d like for the texture array indices to match up with the integers used in the block array in order to make texturing more straightforward.  I think another improvement would be modifying how the block array is generated to allow for vertical expansion as well as horizontal.  I’d also like to improve the player object data structure; I re-used one from a previous project which had some elements I ended up not needing.  It also stored position and velocity as individual values rather than vectors - which I think would’ve been better in hindsight.

## Key Features
*(As of May 2021.)*

### Random World Generation
The world can be resized to any dimension in the horizontal directions, however the vertical depth is currently  limited to 16.  To generate the world, a 3D array with the desired world dimensions stores integers which each indicate a corresponding block.  If an index contains 0, that means there’s no block at that location; if it contains a number between 1-10, that integer corresponds to some block (eg. 1-stone, 2-dirt, etc.).  To populate this array, the indices are looped through and assigned an integer based on their depth.  At a depth of 0, only bedrock blocks are seen.  At a depth of 1, there’s an 8% random chance of a diamond block being placed, otherwise it will be stone.  At each underground depth, the percentage chance of certain ores changes, and the array is filled accordingly.  This array is parsed through when drawing the geometry and the texture of the cube model is determined by the integer at that index in the array. 

To generate the above-world characteristics, grass blocks are generated at a 2.5% chance two layers above the flat ground.  To make the peaks more gradual, there’s then a 50% chance of blocks spawning next to that block on that layer.  On the next layer down, dirt blocks fill the space beneath those grass blocks, and then there’s a 50% chance of grass blocks spreading around those filler blocks.  The ground layer is looped through again to replace any now-covered grass blocks with dirt blocks.  Finally, between 3-8 trees are generated at random positions to finish off the above-ground landscape details.   Because the full resolution Minecraft textures are not publicly available, all textures were recreated using pixel art software and then exported at a higher resolution for use with OpenGL.

<p align="middle">
  <img src="/assets/map3.png" width="48%"/> 
  <img src="/assets/map5.png" width="50%"/> 
</p>

**Images:** 16x16 and 32x32 generated worlds, respectively.

### Placing & Breaking Blocks
Breaking blocks is done by left-clicking and placing blocks is done by right-clicking.  However, currently player’s can only break and place blocks by clicking on the topmost face of any of the block cubes.  With both actions, the first step is taking the player’s current camera position and front-facing direction to determine the coordinate position of where it intersects the horizontal plane corresponding to the topmost-face of whatever block is in the center of the player’s view.  This position must then be converted to the corresponding index of the block to be interacted with (either broken or placed on top of). To do this, I used a recursive function which takes the player’s camera position  and direction and calculates the index of the nearest intersected grid space, starting at the tallest height of all possible grid spaces to avoid accidentally allowing the player to break blocks behind/below the ones currently visible to them.  If this gridspace is empty, then the function recurses at one height depth lower; this continues until it finds a valid block to interact with - it then returns those index values. 

To actually break a block, the index returned by the discussed function is set to 0 in the array which stores all the block data. To place a block, there’s an additional condition that the index above that returned index is empty.  If that is true, then that above index is changed from 0 to whatever block the player is currently holding.  

<img src="/assets/player alg.png" width = "50%"/>

**Image:** The algorithm will calculate hit-point A first, but because A’s corresponding grid space(pink box) is empty, it will then find the player view rays’ intersection at the next depth lower.  Because hit-point B’s block is not empty, the indices of that block space will be returned.  


### Player Movement & Collision
The player views the game in first person, with a small square indicating their “crosshair”, or the center of their screen, and a static block in the bottom right indicating the current block they are placing.  Player movement is conducted using the WASD keys and the mouse, with the mouse rotating the player’s camera and the WASD keys moving the player relative to the direction they’re currently looking in.  The player can also jump using the spacebar.  

Player collision is done by taking the player’s current velocities in the x, y and z directions, applying this velocity to predict their next position, and then using that position when calculating any potential collisions.  The collision calculation works by taking this future position, converting it into the corresponding grid index that the player will be moving towards, and then checking if that position is open.  If it isn’t open, the player’s velocity in that direction will be set to 0 once their position is within a certain radius to that next block’s collision boundary.  

![screenshot of the player's first-person view](/assets/ui.png)
**Image:** First-person player view, with crosshair and current block UI elements highlighted. 

