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
const int MAX_DEPTH = 7;

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
        for (size_t i = 0; i < balls_in_node.size(); i++)
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

    // std::cout << "Grid " << current_depth << parent_node->depth << ". " << "Center: {" << parent_node->position.x << ", " << parent_node->position.y << "}. ";
    // std::cout << "Half: " << parent_node->half_width << ". Min: {" << parent_node->min.x << ", " << parent_node->min.y << "}. Max: {" << parent_node->max.x << ", " << parent_node->max.y << "}.";
    // std::cout << "Leaf: " << parent_node->isLeaf << std::endl;
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
                InsertBallToNode(ball, ball->current_node->child[i]); // Start going down again
            }
        }
    }
    else
    {
        // Sauce: https://stackoverflow.com/questions/41946007/efficient-and-well-explained-implementation-of-a-quadtree-for-2d-collision-det
        // Erases the node's reference to this particular ball
        ball->current_node->RemoveBall(ball);
        // ball->current_node->balls_in_node.erase(std::remove(ball->current_node->balls_in_node.begin(), ball->current_node->balls_in_node.end(), ball), ball->current_node->balls_in_node.end());

        // Changing the ball's reference to the former node's parent. Keep going up until we find an appropriate node
        ball->current_node = ball->current_node->parent;
        UpdateBall(ball);
    }
};

struct cell
{
    Vector2 position = {0.0f, 0.0f};
    Color color = RED;
    Vector2 max;
    Vector2 min;

    std::vector<Ball> ballsInCell;

    bool operator==(const cell &cell)
    {
        return (this->position.x == cell.position.x && this->position.y == cell.position.y);
    }

    bool operator==(const Vector2 &position)
    {
        return (this->position.x == position.x && this->position.y == position.y);
    }
    void addBall(Ball &ball)
    {
        if (std::find(ballsInCell.begin(), ballsInCell.end(), ball) != ballsInCell.end())
        {
        }
        else
        {
            this->ballsInCell.push_back(ball);
        }
    }
    void clearBalls()
    {
        this->ballsInCell.clear();
    }

    bool isEmpty()
    {
        return (this->ballsInCell.size() <= 0);
    }
};

float getDistance(Ball b1, Ball b2)
{
    Vector2 dist = Vector2Subtract(b1.position, b2.position);
    return std::abs(Vector2Length(dist));
}

float getDistanceToPoint(Ball b1, Vector2 pos)
{
    Vector2 dist = Vector2Subtract(b1.position, pos);
    return std::abs(Vector2Length(dist));
}

bool isCirclesColliding(Ball b1, Ball b2)
{
    float sumOfRadii = b1.radius + b2.radius;
    float distance = getDistance(b1, b2);
    if (distance <= sumOfRadii)
    {
        return true;
    }
    return false;
}

Vector2 getNearestIndexAtPoint(Vector2 position)
{ // get the index of the cell (inverted)
    if (position.x < 0)
    {
        return Vector2{0, std::floor(position.y / cellSize)};
    }
    if (position.y < 0)
    {
        return Vector2{std::floor(position.x / cellSize), 0};
    }
    if (position.x < 0 && position.y < 0)
    {
        return Vector2{0, 0};
    }

    if (position.x > WINDOW_WIDTH)
    {
        return Vector2{std::floor((float)WINDOW_WIDTH / cellSize), std::floor(position.y / cellSize)};
    }
    if (position.y > WINDOW_HEIGHT)
    {
        return Vector2{std::floor(position.x / cellSize), std::floor((float)WINDOW_HEIGHT / cellSize)};
    }
    if (position.x > WINDOW_WIDTH && position.y > WINDOW_HEIGHT)
    {
        return Vector2{std::floor((float)WINDOW_WIDTH / cellSize), std::floor((float)WINDOW_HEIGHT / cellSize)};
    }
    return Vector2{std::floor(position.x / cellSize), std::floor(position.y / cellSize)};
}

void initializeCell(cell &testCell, Vector2 pos, Color color)
{
    testCell.position = pos;
    testCell.max = Vector2{testCell.position.x + cellSize, testCell.position.y};
    testCell.min = Vector2{testCell.position.x, testCell.position.y + cellSize};
    testCell.color = color;
}

void initializeAllCells(std::vector<std::vector<cell>> &Cells)
{
    // get array of columns, store it in a row.
    int numberOFRows = std::ceil((float)WINDOW_HEIGHT / (float)cellSize);
    int numberOfColumns = std::ceil((float)WINDOW_WIDTH / (float)cellSize);
    std::cout << numberOFRows << std::endl;
    std::cout << numberOfColumns << std::endl;
    for (int i = 0; i < numberOFRows; i++)
    {
        std::vector<cell> row;
        for (int j = 0; j < numberOfColumns; j++)
        {
            cell x;
            initializeCell(x, Vector2{(float)j * cellSize, (float)i * cellSize}, RED);
            row.push_back(x);
        }
        Cells.push_back(row);
    }
}

void addBallToCell(std::vector<std::vector<cell>> &grid, Ball ball)
{
    Vector2 max = Vector2{ball.position.x + ball.radius, ball.position.y + ball.radius};
    Vector2 min = Vector2{ball.position.x - ball.radius, ball.position.y - ball.radius};

    Vector2 indexAtMin = getNearestIndexAtPoint(min);
    Vector2 indexAtCenter = getNearestIndexAtPoint(ball.position);
    Vector2 indexAtMax = getNearestIndexAtPoint(max);
    Vector2 indexAtLowerRight = getNearestIndexAtPoint(Vector2{max.x, min.y});
    Vector2 indexAtUpperLeft = getNearestIndexAtPoint(Vector2{min.x, max.y});

    // std::cout << "Index At center" << indexAtCenter.x << " " << indexAtCenter.y << std::endl;
    grid[indexAtCenter.y][indexAtCenter.x].addBall(ball);
    grid[indexAtCenter.y][indexAtCenter.x].color = BLUE;

    grid[indexAtMin.y][indexAtMin.x].addBall(ball);
    grid[indexAtMin.y][indexAtMin.x].color = BLUE;

    grid[indexAtMax.y][indexAtMax.x].addBall(ball);
    grid[indexAtMax.y][indexAtMax.x].color = BLUE;

    grid[indexAtLowerRight.y][indexAtLowerRight.x].addBall(ball);
    grid[indexAtLowerRight.y][indexAtLowerRight.x].color = BLUE;

    grid[indexAtUpperLeft.y][indexAtUpperLeft.x].addBall(ball);
    grid[indexAtUpperLeft.y][indexAtUpperLeft.x].color = BLUE;
}

void updateCellContents(std::vector<std::vector<cell>> &grid, std::vector<Ball> &balls)
{
    for (int i = 0; i < grid.size(); i++)
    {
        for (int j = 0; j < grid[i].size(); j++)
        {
            grid[i][j].clearBalls();
            if (grid[i][j].ballsInCell.size() > 0)
            {
                grid[i][j].color = BLUE;
            }
            else
            {
                grid[i][j].color = RED;
            }
        }
    }
    for (int k = 0; k < balls.size(); k++)
    {
        addBallToCell(grid, balls[k]);
    }
}

void checkCollisionInCell(std::vector<std::vector<cell>> &grid, float elasticityCoefficient, std::vector<Ball> &ballArray)
{
    for (int i = 0; i < grid.size(); i++)
    {
        for (int j = 0; j < grid[i].size(); j++)
        {
            for (int k = 0; k < grid[i][j].ballsInCell.size(); k++)
            {

                ballArray[grid[i][j].ballsInCell[k].index].position = Vector2Add(grid[i][j].ballsInCell[k].position, Vector2Scale(grid[i][j].ballsInCell[k].velocity, TIMESTEP));

                if (ballArray[grid[i][j].ballsInCell[k].index].position.x - ballArray[grid[i][j].ballsInCell[k].index].radius <= 0)
                {
                    ballArray[grid[i][j].ballsInCell[k].index].position.x = ballArray[grid[i][j].ballsInCell[k].index].radius;
                    ballArray[grid[i][j].ballsInCell[k].index].velocity.x *= -1;
                }
                if (ballArray[grid[i][j].ballsInCell[k].index].position.x + ballArray[grid[i][j].ballsInCell[k].index].radius >= WINDOW_WIDTH)
                {
                    ballArray[grid[i][j].ballsInCell[k].index].position.x = WINDOW_WIDTH - ballArray[grid[i][j].ballsInCell[k].index].radius;
                    ballArray[grid[i][j].ballsInCell[k].index].velocity.x *= -1;
                }
                if (ballArray[grid[i][j].ballsInCell[k].index].position.y - ballArray[grid[i][j].ballsInCell[k].index].radius <= 0)
                {
                    ballArray[grid[i][j].ballsInCell[k].index].position.y = ballArray[grid[i][j].ballsInCell[k].index].radius;
                    ballArray[grid[i][j].ballsInCell[k].index].velocity.y *= -1;
                }
                if (ballArray[grid[i][j].ballsInCell[k].index].position.y + ballArray[grid[i][j].ballsInCell[k].index].radius >= WINDOW_HEIGHT)
                {
                    ballArray[grid[i][j].ballsInCell[k].index].position.y = WINDOW_HEIGHT - ballArray[grid[i][j].ballsInCell[k].index].radius;
                    ballArray[grid[i][j].ballsInCell[k].index].velocity.y *= -1;
                }

                for (int l = 0; l < grid[i][j].ballsInCell.size(); l++)
                {
                    if (l == k)
                    {
                        continue;
                    }
                    else if (l >= grid[i][j].ballsInCell.size() || k >= grid[i][j].ballsInCell.size())
                    {
                        break;
                    }
                    Vector2 n = Vector2Normalize(Vector2Subtract(ballArray[grid[i][j].ballsInCell[k].index].position, ballArray[grid[i][j].ballsInCell[l].index].position));
                    if (isCirclesColliding(ballArray[grid[i][j].ballsInCell[k].index], ballArray[grid[i][j].ballsInCell[l].index]) && Vector2DotProduct(n, Vector2Subtract(grid[i][j].ballsInCell[k].velocity, grid[i][j].ballsInCell[l].velocity)) < 0)
                    {
                        float impulsej = -((
                                               (1 + elasticityCoefficient) * Vector2DotProduct(Vector2Subtract(ballArray[grid[i][j].ballsInCell[k].index].velocity, ballArray[grid[i][j].ballsInCell[l].index].velocity), n)) /
                                           (Vector2DotProduct(n, n) * (1 / ballArray[grid[i][j].ballsInCell[k].index].mass) + (1 / ballArray[grid[i][j].ballsInCell[l].index].mass)));

                        Vector2 newVelocity = Vector2Add(ballArray[grid[i][j].ballsInCell[k].index].velocity, Vector2Scale(n, (impulsej / ballArray[grid[i][j].ballsInCell[k].index].mass)));
                        ballArray[grid[i][j].ballsInCell[k].index].velocity = newVelocity;
                        Vector2 newVelocity2 = Vector2Subtract(ballArray[grid[i][j].ballsInCell[l].index].velocity, Vector2Scale(n, (impulsej / ballArray[grid[i][j].ballsInCell[l].index].mass)));
                        ballArray[grid[i][j].ballsInCell[l].index].velocity = newVelocity2;
                    }
                }
            }
        }
    }
}

float RandomDirection()
{
    float x = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

    // Make it [-1, 1]
    return x * 2.0f - 1.0f;
}

void InitializeBall(std::vector<Ball> &array, int arraySize, bool isLarge, Node *node, int index)
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
        ball.velocity = {500.0f * RandomDirection(), 500.0f * RandomDirection()};
        // ball.index = index;
        // index++;
        array.push_back(ball);

        InsertBallToNode(&array.back(), node); // BALL INSERTION. Starting with root node as "current node". Balls recursively move down the tree
        // std::cout << "node # " << array.size() << " at quad " << array.back().current_node->position.x << ", " << array.back().current_node->position.y << std::endl;
    }
}

void updateBallsIndex(std::vector<Ball> &ballArray)
{
    for (int i = 0; i < ballArray.size(); i++)
    {
        ballArray[i].index = i;
    }
}

Vector2 getCenterOfRectangle(Vector2 RectanglePos, float width, float height)
{ // ( (x1 + x2) / 2, (y1 + y2) / 2 )
    return Vector2{(RectanglePos.x + width) / 2, (RectanglePos.y + height) / 2};
}

int main()
{
    int elasticityCoefficient = 1.0f;

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "OlivaresTamano - Homework 4");

    SetTargetFPS(FPS);

    float accumulator = 0;

    std::vector<Ball> ballArray;
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
    int ind = 0;

    while (!WindowShouldClose())
    {

        float delta_time = GetFrameTime();
        Vector2 forces = Vector2Zero();
        // Vector2 mouseIndexLocation = getNearestIndexAtPoint(GetMousePosition());
        // if (IsMouseButtonDown(0))
        // {
        //     // std::cout << "MOUSE INDEX: " << mouseIndexLocation.x << " " << mouseIndexLocation.y << std::endl;
        // }

        //  updateBallsIndex(ballArray);
        //  updateCellContents(grid, ballArray);
        //  if (IsKeyPressed(KEY_TAB))
        //  {
        //      drawGrid = !drawGrid;
        //  }

        // A ball is initialized and inserted into the appropriate node
        if (IsKeyPressed(KEY_SPACE))
        {
            if (spawnInstance == 10)
            {
                InitializeBall(ballArray, 1, true, &root_node, ind);
                ind++;
                spawnInstance = 0;
            }
            else
            {
                InitializeBall(ballArray, 25, false, &root_node, ind);
                ind += 25;
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

                UpdateBall(&current_ball);
                // InsertBallToNode(&current_ball, &root_node); // Update ball
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
        //     for (int i = 0; i < grid.size(); i++)
        //     {
        //         for (int j = 0; j < grid[i].size(); j++)
        //         {
        //             const char *numberOfBalsInCell = std::to_string(grid[i][j].ballsInCell.size()).c_str();
        //             Vector2 rectMidpoint = Vector2{getCenterOfRectangle(grid[i][j].position, cellSize, cellSize)};
        //             DrawText(numberOfBalsInCell, grid[i][j].max.x, grid[i][j].max.y, 5, PURPLE);
        //             DrawRectangleLines(grid[i][j].position.x, grid[i][j].position.y, cellSize, cellSize, grid[i][j].color);
        //         }
        //     }
        // }

        // if (drawGrid)
        // {
        DrawQuadTree(&root_node);
        // }

        EndDrawing();
    }
    CloseWindow();
    return 0;
}