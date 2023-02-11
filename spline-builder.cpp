#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <math.h>

struct Point {
	float x;
	float y;
};

struct Node : Point {
	bool hasHandleA, hasHandleB;
	Point handleA;
	Point handleB;
};

// Structure representing a selected node
// index: The index of the node in the list
// controlPoint: Which point is selected - 0: the node itself, 1: point A, 2: point B
struct NodeSelection {
	int index;
	int controlPoint;
};

std::vector<Node> nodes = {};
bool hasClicked = false, drag = false;
const float POINT_SIZE = 20;
const float LINE_WIDTH = 5.0f;
const int CURVE_SEGMENTS = 200;
double mx = 0, my = 0;
double *mouseX = &mx, *mouseY = &my;

// Calculates the distance between two points
float distance(float x1, float y1, float x2, float y2){
	float dx = x2 - x1;
	float dy = y2 - y1;
	return sqrtf32(dx * dx + dy * dy);
}

// Returns true if the mouse is over the point
bool mouseOver(Point p){
	return distance(p.x, p.y, (float) *mouseX, (float) *mouseY) < POINT_SIZE;
}

// Returns a NodeSelection for the closest existing node or control point to the given point
// If endsOnly is true, only consider the end nodes
// If nodesOnly is true, only consider nodes (not control points)
NodeSelection closestNodePoint(float x, float y, bool endsOnly, bool nodesOnly){
	NodeSelection ns;
	Point closest;
	closest.x = MAXFLOAT;
	closest.y = MAXFLOAT;
	for (int i = 0; i < nodes.size(); i++){
		if (endsOnly && i != 0 && i != nodes.size() - 1){
			// If in "ends only" mode, skip nodes with both handles (which are always interior nodes)
			continue;
		}
		if (distance(x, y, nodes[i].x, nodes[i].y) < distance(x, y, closest.x, closest.y)){
			closest = nodes[i];
			ns.index = i;
			ns.controlPoint = 0;
		}
		// If "nodes only" mode is off, also check the node's control points
		if (!nodesOnly){
			if (nodes[i].hasHandleA && distance(x, y, nodes[i].handleA.x, nodes[i].handleA.y) < distance(x, y, closest.x, closest.y)){
				closest = nodes[i].handleA;
				ns.index = i;
				ns.controlPoint = 1;
			}
			if (nodes[i].hasHandleB && distance(x, y, nodes[i].handleB.x, nodes[i].handleB.y) < distance(x, y, closest.x, closest.y)){
				closest = nodes[i].handleB;
				ns.index = i;
				ns.controlPoint = 2;
			}
		}
		
	}
	return ns;
}

// Returns a NodeSelection for the node or control point that the mouse is over (index will be -1 if the mouse is not over a node or control point)
NodeSelection selectedNode(){
	NodeSelection ns;
	ns.index = -1;
	if (nodes.size() == 0){
		return ns;
	}

	NodeSelection n = closestNodePoint(*mouseX, *mouseY, false, false);
	if (mouseOver(nodes[n.index]) || (nodes[n.index].hasHandleA && mouseOver(nodes[n.index].handleA)) || (nodes[n.index].hasHandleB && mouseOver(nodes[n.index].handleB))){
		return n;
	}
	return ns;
}

// Returns the position of the reflection of the point p across the centre point c
Point mirrorAcross(Point p, Point c){
	float dx = c.x - p.x;
	float dy = c.y - p.y;
	Point r;
	r.x = c.x + dx;
	r.y = c.y + dy;
	return r;
}

// Adds a node to the end of the list, enabling this node's handle A and the previous end node's handle B
void addNodeToEnd(Node n){
	// Assign the single handle as handle A
	n.hasHandleA = true;
	Point p;
	p.x = n.x;
	p.y = n.y + 50;
	n.handleA = p;

	// Set coordinates of handle B but don't enable it
	Point b = mirrorAcross(p, n);
	n.handleB = b;
	n.hasHandleB = false;

	// Enable handle B for the current end node
	nodes[nodes.size() - 1].hasHandleB = true;

	// Add the new node to the end of the list
	nodes.push_back(n);

}

// Adds a node to the beginning of the list, enabling this node's handle B and the previous start node's handle B
void addNodeToStart(Node n){
	// Assign the single handle as handle B
	n.hasHandleB = true;
	Point p;
	p.x = n.x;
	p.y = n.y + 50;
	n.handleB = p;

	// Set the coordinates of handle A but don't enable it
	Point a = mirrorAcross(p, n);
	n.handleA = a;
	n.hasHandleA = false;

	// Enable handle A for the current start node
	nodes[0].hasHandleA = true;

	// Add the new node to the start of the list
	nodes.insert(nodes.cbegin(), n);
}

// Check whether two points are equal
bool pointEquals(Point a, Point b){
	return (a.x == b.x && a.y == b.y);
}

// Calculates the point along a cubic Bezier curve between points a and b with control points c1 and c2 at value t
Point cubicBezier(Point a, Point b, Point c1, Point c2, float t){
	Point result;
	result.x = (a.x)*(powf32(1-t, 3)) + 3*(c1.x)*t*powf32(1-t, 2) + 3*(c2.x)*(1-t)*t*t + (b.x)*powf32(t, 3);
	result.y = (a.y)*(powf32(1-t, 3)) + 3*(c1.y)*t*powf32(1-t, 2) + 3*(c2.y)*(1-t)*t*t + (b.y)*powf32(t, 3);
	return result;
}

// Draws a Bezier curve between nodes A and B with (segments) segments.
void bezierBetween(Node a, Node b, int segments){
	glBegin(GL_LINE_STRIP);
		for (float t = 0; t < 1.0f ; t += (1.0f / (float)segments)){
			Point p = cubicBezier(a, b, a.handleB, b.handleA, t);
			glVertex2f(p.x, p.y);
		}
	glEnd();
}


int main(int argc, char* argv[]){
	NodeSelection currentNode;
	currentNode.index = -1;
	currentNode.controlPoint = 0;

	// Get command line arguments
	int w = 1000, h = 1000;
	if (argc > 3 || argc == 2){
		std::cout << "Wrong number of arguments. Correct usage: spline-builder W H" << std::endl;
		return -1;
	}
	else if (argc == 3){
		try{
			w = std::stoi(argv[1]);
			h = std::stoi(argv[2]);
		}
		catch (...){
			std::cout << "W and H arguments must be integers" << std::endl;
			return -1;
		}
	}
	if (w <= 0 || h <= 0){
		std::cout << "W and H arguments must be positive" << std::endl;
		return -1;
	}
	
	// Initialize window
	GLFWwindow* window;
	if (!glfwInit()){
		return -1;
	}

	// Setup multisampling and viewport before the window is created
	glfwWindowHint(GLFW_SAMPLES, 4);	// 4x multisampling
	glEnable(GL_MULTISAMPLE);

	glMatrixMode(GL_VIEWPORT);
	glLoadIdentity();
	glViewport(0, 0, w, h);

	window = glfwCreateWindow(w, h, "Spline Builder", NULL, NULL);
	if (!window){
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Setup projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, 0, h, -1, 1);
	
	glClearColor(1, 1, 1, 1);
	glPointSize(POINT_SIZE);

	while (!glfwWindowShouldClose(window)){
		glfwPollEvents();
		glfwGetCursorPos(window, mouseX, mouseY);
		*mouseY = h - *mouseY;	// Set mouse Y position to the correct coordinate system
		glClear(GL_COLOR_BUFFER_BIT);

		// Key press handling
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS){
			nodes.clear();
		}

		// Mouse click handler
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS){
			if (!hasClicked){
				// The mouse button state changed on this frame. Either place a node or start dragging a node.
				hasClicked = true;
				// Check whether mouse is over any node or control point
				NodeSelection node = selectedNode();
				if (node.index == -1){
					// If not, place a node
					// Create node and place it at the mouse position
					Node n;
					n.x = (float) *mouseX;
					n.y = (float) *mouseY;

					// Determine which end it should connect to
					// If there are no nodes yet, skip this step
					if (nodes.size() != 0){
						// If there's only one other node, place this node after it
						if (nodes.size() == 1){
							addNodeToEnd(n);
						}
						else{
							// Get the closest end node to the new node
							// Should always be nodes[0] or nodes.back
							Node end = nodes[closestNodePoint(n.x, n.y, true, true).index];
							if (pointEquals(end, nodes.back())){
								addNodeToEnd(n);
							}
							else{
								addNodeToStart(n);
							}
						}
					}
					else {
						// The list is empty, so add a node with only handle A and don't connect it to anything
						n.hasHandleA = true;
						Point a;
						a.x = n.x;
						a.y = n.y + 50;
						n.handleA = a;
						n.handleB = mirrorAcross(a, n);
						n.hasHandleB = false;

						nodes.push_back(n);
					}
					std::cout << "Node placed at " << n.x << ", " << n.y << std::endl;
				}
				else {
					// Start dragging the node/control point
					drag = true;
					currentNode = node;
				}
				
				
			}
			else if (drag){
				if (currentNode.index == -1){
					// The drag flag is set but no node is selected.
					// Shouldn't happen but handling it just in case
					drag = false;
				}
				else if (currentNode.controlPoint == 0) {
					// Dragging a node
					// Save the offsets of the node's control points
					float offsAX = nodes[currentNode.index].handleA.x - nodes[currentNode.index].x;
					float offsAY = nodes[currentNode.index].handleA.y - nodes[currentNode.index].y;

					float offsBX = nodes[currentNode.index].handleB.x - nodes[currentNode.index].x;
					float offsBY = nodes[currentNode.index].handleB.y - nodes[currentNode.index].y;
					
					// Move the selected node to the exact position of the mouse
					nodes[currentNode.index].x = (float) *mouseX;
					nodes[currentNode.index].y = (float) *mouseY;

					// Move the control points to the same offset from the new position
					nodes[currentNode.index].handleA.x = nodes[currentNode.index].x + offsAX;
					nodes[currentNode.index].handleA.y = nodes[currentNode.index].y + offsAY;

					nodes[currentNode.index].handleB.x = nodes[currentNode.index].x + offsBX;
					nodes[currentNode.index].handleB.y = nodes[currentNode.index].y + offsBY;
				}
				else{
					// Dragging a control point
					if (currentNode.controlPoint == 1){
						// Moving control point A
						// Move it to the position of the mouse cursor
						nodes[currentNode.index].handleA.x = (float) *mouseX;
						nodes[currentNode.index].handleA.y = (float) *mouseY;

						// Move point B to the reflection of A across the node
						nodes[currentNode.index].handleB = mirrorAcross(nodes[currentNode.index].handleA, nodes[currentNode.index]);
					}
					else {
						// Moving control point B
						// Move it to the position of the mouse cursor
						nodes[currentNode.index].handleB.x = (float) *mouseX;
						nodes[currentNode.index].handleB.y = (float) *mouseY;

						// Move point B to the reflection of A across the node
						nodes[currentNode.index].handleA = mirrorAcross(nodes[currentNode.index].handleB, nodes[currentNode.index]);
					}
				}
			}
		}
		else{
			hasClicked = false;
			drag = false;
			currentNode.index = -1;
		}

		glColor3f(0, 0, 0);
		NodeSelection s = selectedNode();	// Save this in a variable so the function only has to get called once

		// Draw the lines between the handles and the nodes		
		glEnable(GL_LINE_STIPPLE);
		glLineStipple(1, 0x3F3F);
		glBegin(GL_LINES);
			for (int i = 0; i < nodes.size(); i++){
				if (nodes[i].hasHandleA){
					glVertex2f(nodes[i].x, nodes[i].y);
					glVertex2f(nodes[i].handleA.x, nodes[i].handleA.y);
				}
				if (nodes[i].hasHandleB){
					glVertex2f(nodes[i].x, nodes[i].y);
					glVertex2f(nodes[i].handleB.x, nodes[i].handleB.y);
				}
			}
		glEnd();
		glDisable(GL_LINE_STIPPLE);

		// Draw the curves
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_BLEND);
		glLineWidth(LINE_WIDTH);
		if (nodes.size() > 1){
			for (int i = 0; i < nodes.size() - 1; i++){
				bezierBetween(nodes[i], nodes[i + 1], CURVE_SEGMENTS);
			}
		}
		glDisable(GL_LINE_SMOOTH);
		glLineWidth(1.0f);
		
		// Draw the nodes
		glDisable(GL_POINT_SMOOTH);
		glBegin(GL_POINTS);
			for (int i = 0; i < nodes.size(); i++){
				Node n = nodes[i];
				// Highlight the node if it's selected
				if (i == s.index && s.controlPoint == 0){
					glColor3f(0.5f, 0.5f, 0.5f);
				}
				glVertex2f(n.x, n.y);
				glColor3f(0, 0, 0);
			}
		glEnd();

		// Draw the handles
		glColor3f(0, 0, 1);
		glEnable(GL_POINT_SMOOTH);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBegin(GL_POINTS);
			for (int i = 0; i < nodes.size(); i++){
				Node n = nodes[i];

				if (n.hasHandleA){
					// Highlight the handle if selected
					if (s.index == i && s.controlPoint == 1){
						glColor3f(0.5f, 0.5f, 1);
					}
					glVertex2f(n.handleA.x, n.handleA.y);
					glColor3f(0, 0, 1);
				}
				if (n.hasHandleB){
					// Highlight the handle if selected
					if (s.index == i && s.controlPoint == 2){
						glColor3f(0.5f, 0.5f, 1);
					}
					glVertex2f(n.handleB.x, n.handleB.y);
				}

				glColor3f(0, 0, 1);
			}
		glEnd();
		glDisable(GL_POINT_SMOOTH);

		glfwSwapBuffers(window);

		
	}
	
	return 0;
}