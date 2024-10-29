//  g++ Main.cpp -o Main -I raylib/ -L raylib/ -lraylib -lopengl32 -lgdi32 -lwinmm
//  ./Main.exe
#include <raylib.h>
#include <raymath.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const float FPS = 60;
const float TIMESTEP = 1 / FPS;
const int cellSize = 50;
const int MAX_DEPTH = 4;

struct Node;
struct Ball;
struct cell;

struct Node
{
    Vector2 position; // AKA Center. If root, {WINDOW_WIDTH/2,WINDOW_HEIGHT/2}. If child, (parent.half_width/2)+-parent.position), (parent.halfwidth/2)+-parent.position)}
    float half_width; // If root node, half of the screen's width or height, whichever is greater. If child, parent.half_width
    Vector2 min, max; // If, for any axis, the minimum coordinate value of one is greater than the maximum coordinate of the other, then there is no intersection
    bool isLeaf;
    int depth;                         // Up to 3
    Node *parent;                      // Empty if root
    Node *child[4];                    // 0 = upper left, 1 = upper right, 2 = lower left, 3 = lower right
    std::vector<Ball *> balls_in_node; // List of objects in node

    // Removes the specific ball from the node
    void RemoveBall(Ball *ball)
    {
        for (int i = 0; i < balls_in_node.size(); i++)
        {
            if (balls_in_node[i] == ball)
            {
                balls_in_node.erase(balls_in_node.begin() + i);
                break;
            }
        }
    }
};

struct Ball
{
    Node *current_node;
    Vector2 position;
    float radius;
    Color color;
    int index;
    float mass;
    float inverse_mass;
    Vector2 velocity;

    bool operator==(const Ball &ball)
    {
        return (
            this->position.x == ball.position.x &&
            this->position.y == ball.position.y &&
            this->radius == ball.radius &&
            this->index == ball.index &&
            this->mass == ball.mass);
    }

    bool operator!=(const Ball &ball)
    {
        return (
            this->position.x != ball.position.x &&
            this->position.y != ball.position.y &&
            this->radius != ball.radius &&
            this->index != ball.index &&
            this->mass != ball.mass);
    }
};

void InitializeNodes(Node *parent_node, int current_depth)
{
    // std::cout << "Grid " << current_depth << " being initialized" << std::endl;
    if (current_depth < MAX_DEPTH) // From root node to MAX_DEPTH-1
    {
        parent_node->isLeaf = false;
        parent_node->child[0] = new Node;
        parent_node->child[1] = new Node;
        parent_node->child[2] = new Node;
        parent_node->child[3] = new Node;

        for (int y = 0; y < 2; y++) // y 0 = upper, y 1 = lower
        {
            for (int x = 0; x < 2; x++) // x 0 = left, x 1 = right
            {
                int i = (y * 2) + x; // upper left -> upper right -> lower left -> lower right
                parent_node->child[i]->half_width = parent_node->half_width / 2;

                // Determining what kind of child you are
                Vector2 child_position;
                if (i == 0)
                {
                    child_position = {parent_node->position.x - parent_node->child[i]->half_width, parent_node->position.y - parent_node->child[i]->half_width};
                }
                else if (i == 1)
                {
                    child_position = {parent_node->position.x + parent_node->child[i]->half_width, parent_node->position.y - parent_node->child[i]->half_width};
                }
                else if (i == 2)
                {
                    child_position = {parent_node->position.x - parent_node->child[i]->half_width, parent_node->position.y + parent_node->child[i]->half_width};
                }
                else if (i == 3)
                {
                    child_position = {parent_node->position.x + parent_node->child[i]->half_width, parent_node->position.y + parent_node->child[i]->half_width};
                }

                parent_node->child[i]->position = child_position;
                parent_node->child[i]->min = {child_position.x - parent_node->child[i]->half_width, child_position.y - parent_node->child[i]->half_width};
                parent_node->child[i]->max = {child_position.x + parent_node->child[i]->half_width, child_position.y + parent_node->child[i]->half_width};
                parent_node->child[i]->depth = current_depth + 1;
                parent_node->child[i]->parent = parent_node;

                // Keep making nodes until maximum depth
                InitializeNodes(parent_node->child[i], current_depth + 1);
            }
        }
    }
    else if (current_depth == MAX_DEPTH) // For the leaf nodes
    {
        parent_node->isLeaf = true;
    }
}

void UpdateNodes(Node *current_node, int current_depth)
{
    if (!current_node->balls_in_node.empty())
    {
        current_node->balls_in_node.clear();
    }

    if (current_depth < MAX_DEPTH) // From root node to MAX_DEPTH-1
    {
        for (int i = 0; i < 4; i++)
        {
            UpdateNodes(current_node->child[i], current_depth + 1);
        }
    }
}

void InsertBallToNode(Ball *ball, Node *node)
{

    for (int i = 0; i < 4; i++)
    {
        if (!node->isLeaf &&
            node->child[i]->max.x >= ball->position.x + ball->radius &&
            node->child[i]->max.y >= ball->position.y + ball->radius &&
            node->child[i]->min.x <= ball->position.x - ball->radius &&
            node->child[i]->min.y <= ball->position.y - ball->radius)
        {
            InsertBallToNode(ball, node->child[i]);
            return; // End this function if a smaller cell is found so the ball won't get included in the parent node's list
        }
    }
    node->balls_in_node.push_back(ball);
    ball->current_node = node;
}

void DrawQuadTree(Node *current_node)
{

    if (!current_node->balls_in_node.empty())
    {
        // std::cout << "Node depth " << current_node->depth << "has" << current_node->balls_in_node.size() << "balls" << std::endl;
        const char *numberOfBalsInCell = std::to_string(current_node->balls_in_node.size()).c_str();
        DrawText(numberOfBalsInCell, current_node->max.x - current_node->half_width * 1.05, current_node->max.y - current_node->half_width * 1.15, 20, PURPLE);
        DrawRectangleLines(current_node->min.x, current_node->min.y, current_node->half_width * 2, current_node->half_width * 2, BLACK);
    }
    if (!current_node->isLeaf)
    {
        for (int i = 0; i < 4; i++)
        {
            DrawQuadTree(current_node->child[i]);
        }
    }
};

void UpdateBall(Ball *ball)
{

    if ( // If the object is in a node that covers it completely
        ball->current_node->max.x >= ball->position.x + ball->radius &&
        ball->current_node->max.y >= ball->position.y + ball->radius &&
        ball->current_node->min.x <= ball->position.x - ball->radius &&
        ball->current_node->min.y <= ball->position.y - ball->radius)
    {
        for (int i = 0; i < 4; i++)
        {
            // If any of the children of the object's current node can cover the object completely
            if (!ball->current_node->isLeaf &&
                ball->current_node->child[i]->max.x >= ball->position.x + ball->radius &&
                ball->current_node->child[i]->max.y >= ball->position.y + ball->radius &&
                ball->current_node->child[i]->min.x <= ball->position.x - ball->radius &&
                ball->current_node->child[i]->min.y <= ball->position.y - ball->radius)
            {
                // ball->current_node->balls_in_node.erase(std::remove(ball->current_node->balls_in_node.begin(), ball->current_node->balls_in_node.end(), ball), ball->current_node->balls_in_node.end());
                ball->current_node->RemoveBall(ball);
                ball->current_node = ball->current_node->child[i];
                InsertBallToNode(ball, ball->current_node); // Start going down again
            }
        }
    }
    else
    {
        // Sauce: https://stackoverflow.com/questions/41946007/efficient-and-well-explained-implementation-of-a-quadtree-for-2d-collision-det
        // Erases the node's reference to this particular ball
        // ball->current_node->RemoveBall(ball);
        ball->current_node->balls_in_node.erase(std::remove(ball->current_node->balls_in_node.begin(), ball->current_node->balls_in_node.end(), ball), ball->current_node->balls_in_node.end());

        // Changing the ball's reference to the former node's parent. Keep going up until we find an appropriate node
        ball->current_node = ball->current_node->parent;
        UpdateBall(ball);
    }
};

void CheckBallCollision(Ball *ball, Node *current_node)
{
    for (int i = 0; i < current_node->balls_in_node.size(); i++)
    {
        if (ball != current_node->balls_in_node[i])
        {
            Vector2 n = Vector2Subtract(ball->position, current_node->balls_in_node[i]->position); // Collision Normal
            float distance = Vector2Length(n);
            float sum_of_radii = ball->radius + current_node->balls_in_node[i]->radius;

            Vector2 relative_velocity = Vector2Subtract(ball->velocity, current_node->balls_in_node[i]->velocity);
            float relative_velocity_dot = Vector2DotProduct(n, relative_velocity);

            if (distance <= sum_of_radii && relative_velocity_dot < 0.0f)
            {
                float impulse = -((1 + 1.0f) * Vector2DotProduct(relative_velocity, n)) / (Vector2DotProduct(n, n) * (ball->inverse_mass + current_node->balls_in_node[i]->inverse_mass));
                ball->velocity = Vector2Add(ball->velocity, Vector2Scale(n, impulse * ball->inverse_mass));
                current_node->balls_in_node[i]->velocity = Vector2Subtract(current_node->balls_in_node[i]->velocity, Vector2Scale(n, impulse * current_node->balls_in_node[i]->inverse_mass));
            }
        }
    }
    if (!current_node->isLeaf) // Check if the occupied node is not a parent
    {
        for (int i = 0; i < 4; i++)
        {
            // if (!current_node->child[i]->balls_in_node.empty()) // Checking if there's balls
            // {
            CheckBallCollision(ball, current_node->child[i]);
            // }
        }
    }
};

float RandomDirection()
{
    float x = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

    // Make it [-1, 1]
    return x * 2.0f - 1.0f;
}

void InitializeBall(std::vector<Ball> &array, int arraySize, bool isLarge, Node *node)
{
    for (size_t i = 0; i < arraySize; i++)
    {
        Ball ball;
        Color randomColor = {
            GetRandomValue(0, 255),
            GetRandomValue(0, 255),
            GetRandomValue(0, 255),
            255};
        ball.position = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};
        // ball.position = {(float)GetRandomValue(20, 1245), (float)GetRandomValue(20, 695)};
        if (isLarge)
        {
            ball.radius = 25.0f;
            ball.mass = 10.0f;
            ball.inverse_mass = 1.0f / 10.0f;
        }
        else
        {
            ball.radius = (float)GetRandomValue(5, 10);
            ball.mass = 1.0f;
            ball.inverse_mass = 1.0f;
        }
        ball.color = randomColor;
        ball.velocity = {250.0f * RandomDirection(), 250.0f * RandomDirection()};
        // ball.index = index;
        // index++;
        array.push_back(ball);

        InsertBallToNode(&array.back(), node); // BALL INSERTION. Starting with root node as "current node". Balls recursively move down the tree
        // std::cout << "node # " << array.size() << " at quad " << array.back().current_node->position.x << ", " << array.back().current_node->position.y << std::endl;
    }
}

int main()
{
    int elasticityCoefficient = 1.0f;

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "OlivaresTamano - Homework 4");

    SetTargetFPS(FPS);

    float accumulator = 0;

    std::vector<Ball> ballArray;
    ballArray.reserve(10000);
    int spawnInstance = 0;

    // initialize Cell
    Node root_node;
    root_node.position = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};
    root_node.half_width = (WINDOW_WIDTH > WINDOW_HEIGHT) ? WINDOW_WIDTH / 2 : WINDOW_HEIGHT / 2;
    root_node.min = {root_node.position.x - root_node.half_width, root_node.position.y - root_node.half_width};
    root_node.max = {root_node.position.x + root_node.half_width, root_node.position.y + root_node.half_width};
    root_node.isLeaf = false;
    root_node.depth = 0;

    InitializeNodes(&root_node, 0); // Recursively creates the whole tree
    bool drawGrid = false;

    while (!WindowShouldClose())
    {
        // UpdateNodes(&root_node, 0); // recreate the quadtree and reinsert all the objects with their updated positions per frame
        DrawFPS(WINDOW_WIDTH - 75, 10);
        float delta_time = GetFrameTime();
        Vector2 forces = Vector2Zero();

        //  updateBallsIndex(ballArray);
        //  updateCellContents(grid, ballArray);
        if (IsKeyPressed(KEY_TAB))
        {
            drawGrid = !drawGrid;
        }

        // A ball is initialized and inserted into the appropriate node
        if (IsKeyPressed(KEY_SPACE))
        {
            if (spawnInstance == 10)
            {
                InitializeBall(ballArray, 1, true, &root_node);
                spawnInstance = 0;
            }
            else
            {
                InitializeBall(ballArray, 25, false, &root_node);
                spawnInstance++;
            }
        }

        // Physics
        accumulator += delta_time;
        while (accumulator >= TIMESTEP)
        {
            for (int i = 0; i < ballArray.size(); i++)
            {
                Ball &current_ball = ballArray[i];
                current_ball.position = Vector2Add(current_ball.position, Vector2Scale(current_ball.velocity, TIMESTEP));

                // Screen Edge Collision
                if (current_ball.position.x - current_ball.radius <= 0.0f)
                {
                    current_ball.position.x = current_ball.radius;
                    current_ball.velocity.x *= -1.0f;
                }
                if (current_ball.position.x + current_ball.radius >= WINDOW_WIDTH)
                {
                    current_ball.position.x = WINDOW_WIDTH - current_ball.radius;
                    current_ball.velocity.x *= -1.0f;
                }
                if (current_ball.position.y - current_ball.radius <= 0.0f)
                {
                    current_ball.position.y = current_ball.radius;
                    current_ball.velocity.y *= -1.0f;
                }
                if (current_ball.position.y + current_ball.radius >= WINDOW_HEIGHT)
                {
                    current_ball.position.y = WINDOW_HEIGHT - current_ball.radius;
                    current_ball.velocity.y *= -1.0f;
                }

                UpdateBall(&current_ball); // Updates the ball's current node

                // InsertBallToNode(&current_ball, &root_node); // Update ball
                CheckBallCollision(&current_ball, current_ball.current_node);
            }

            // checkCollisionInCell(grid, elasticityCoefficient, ballArray);
            accumulator -= TIMESTEP;
        }

        const char *numberOfBalls = std::to_string(ballArray.size()).c_str();
        BeginDrawing();
        ClearBackground(WHITE);
        DrawText(numberOfBalls, 0, 0, 30, YELLOW);
        for (int i = 0; i < ballArray.size(); i++)
        {
            DrawCircleV(ballArray[i].position, ballArray[i].radius, ballArray[i].color);
        }
        // if (drawGrid)
        // {
        DrawQuadTree(&root_node);
        // }

        EndDrawing();
    }
    CloseWindow();
    return 0;
}