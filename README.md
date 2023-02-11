# CS3388 Assignment 3
## Submitted files
- `spline-builder.cpp`: Source code for the assignment.
- `demo.mp4`: Screen capture of program operation.
- `README.md`: Idk

## Program operation
- Once compiled, launch the program using `spline-builder W H`, where:
	- `W` is the desired window width (must be a positive integer)
	- `H` is the desired window height (must be a positive integer)
	- If launched with no arguments, the window will default to 1000x1000 (this was just to make it convenient to test and debug)
- Left click on any empty space within the window to place a new node. If other nodes exist, the new node will connect to whichever endpoint of the spline is closest.
- Left click and drag a node to move it.
- Left click and drag a blue control point to move it. The other control point for the same node will move to stay directly across the node from the one you're moving.
- Press E on the keyboard to erase all existing nodes and start over.

## Program notes and source code explanation
### Data structures
- Point: represents a single (x, y) coordinate
- Node: Extension of Point; has an (x, y) coordinate, two control points, and boolean flags to enable each control point. Represents a node in the spline
	- Control point A points "backward" in the spline, control point B points "forward"
- NodeSelection: Contains an integer corresponding to an index in the main node list, and another integer selecting either the node itself or one of the control points. Used to allow the selectedNode function (described later) to return a control point while maintaining its association with its parent node, which is useful when moving the control points.
- The nodes are stored in a global std::vector, so an integer index is enough to get all information about a node and its control points.

### Important functions
- `mirrorAcross`: Given a point P and a centre point C, returns a point corresponding to the reflection of P across C. Used when dragging a control point, to set the position of the other control point accordingly.
- `selectedNode`: Returns a `NodeSelection` corresponding to the node or control point that the mouse is hovering over. Used to determine whether to place a new node or drag an existing node, and also to highlight points when you mouse over them.
- `cubicBezier`: Given two endpoints, two control points, and a `t` value between 0 and 1, calculates the cubic interpolation between the endpoints for the given value of `t`.
- `bezierBetween`: Given two Nodes A and B, and a number of segments N, draws a cubic Bezier curve between them with N segments using control point B of node A and control point A of node B by calling `cubicBezier` for N evenly spaced `t` values between 0 and 1.

### General program operation
- First, the command line arguments are read and checked, and the window is initialized.
- At the start of the main loop, the program gets the mouse position, then checks for keyboard and mouse events.
	- If E is pressed, the node list is cleared.
	- If left click is pressed, and it wasn't last frame, then either place a node or start dragging a node/control point depending on whether the mouse is over one.
	- If left click is being held and something is being dragged, move the dragged point to the mouse position (and if it's a control point, also move its counterpart using `mirrorAcross`)
- After mouse stuff is handled, draw all of the nodes, handles, and the lines connecting them, plus the Bezier curves between them.
	- The instructions call for the nodes to be square, but I cannot for the life of me figure out how to stop them from being round. No matter where I put `glDisable(GL_POINT_SMOOTH)`, or which order I draw the nodes and handles in, the nodes are still round. Even if I remove the `glEnable(GL_POINT_SMOOTH)` call, they're STILL ROUND! What is happening?! 
- There is more explanation in the source code comments.