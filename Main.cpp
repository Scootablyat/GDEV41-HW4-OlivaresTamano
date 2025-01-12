//  g++ Main.cpp -o Main -I raylib/ -L raylib/ -lraylib -lopengl32 -lgdi32 -lwinmm
//  ./Main.exe
#include <raylib.h>
#include <raymath.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const float FPS = 60;
const float TIMESTEP = 1 / FPS;
const int cellSize = 50;

struct Ball
{
    Vector2 position;
    float radius;
    Color color;
    int index;
    float mass;
    float inverse_mass;
    Vector2 velocity;


    bool operator==(const Ball& ball){
        return(
            this->position.x == ball.position.x &&
            this->position.y == ball.position.y &&
            this->radius == ball.radius &&
            this->index == ball.index &&
            this->mass == ball.mass
        );
    }

    bool operator!=(const Ball& ball){
        return(
            this->position.x != ball.position.x &&
            this->position.y != ball.position.y &&
            this->radius != ball.radius &&
            this->index != ball.index &&
            this->mass != ball.mass
        );
    }
};

struct cell{
    Vector2 position = {0.0f, 0.0f};
    Color color = RED;
    Vector2 max;
    Vector2 min;
    
    std::vector<Ball> ballsInCell;

    bool operator==(const cell& cell){
        return (this->position.x == cell.position.x && this->position.y == cell.position.y);
    }

    bool operator==(const Vector2& position){
        return (this->position.x == position.x && this->position.y == position.y);
    }
    void addBall(Ball& ball){
        if(std::find(ballsInCell.begin(), ballsInCell.end(), ball) != ballsInCell.end()){

        }
        else{
            this->ballsInCell.push_back(ball);
        }
    }
    void clearBalls(){
        this->ballsInCell.clear();
    }

    bool isEmpty(){
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

Vector2 getNearestIndexAtPoint(Vector2 position){ // get the index of the cell (inverted)
    if(position.x < 0){
        return Vector2{0, std::floor(position.y/cellSize)};
    }
    if(position.y < 0){
        return Vector2{std::floor(position.x/cellSize), 0};
    }
    if(position.x < 0 && position.y < 0){
        return Vector2{0, 0};
    }

    if(position.x > WINDOW_WIDTH){
        return Vector2{std::floor((float)WINDOW_WIDTH/cellSize), std::floor(position.y/cellSize)};
    }
    if(position.y > WINDOW_HEIGHT){
        return Vector2{std::floor(position.x/cellSize), std::floor((float)WINDOW_HEIGHT/cellSize)};
    }
    if(position.x > WINDOW_WIDTH && position.y > WINDOW_HEIGHT){
        return Vector2{std::floor((float)WINDOW_WIDTH/cellSize), std::floor((float)WINDOW_HEIGHT/cellSize)};
    }
    return Vector2{std::floor(position.x/cellSize), std::floor(position.y/cellSize)};
}

void initializeCell(cell &testCell, Vector2 pos, Color color){ 
    testCell.position = pos;
    testCell.max = Vector2{testCell.position.x + cellSize, testCell.position.y};
    testCell.min = Vector2{testCell.position.x, testCell.position.y + cellSize};
    testCell.color = color;
}

void initializeAllCells(std::vector<std::vector<cell>> &Cells){ 
    // get array of columns, store it in a row.
    int numberOFRows = std::ceil((float)WINDOW_HEIGHT/(float)cellSize);
    int numberOfColumns = std::ceil((float)WINDOW_WIDTH/(float)cellSize);
    std::cout << numberOFRows << std::endl;
    std::cout << numberOfColumns << std::endl;
    for (int i = 0; i < numberOFRows; i++){
        std::vector<cell> row;
        for (int j = 0; j < numberOfColumns; j++){
            cell x;
            initializeCell(x, Vector2{(float) j*cellSize, (float) i*cellSize}, RED);
            row.push_back(x); 
        }
        Cells.push_back(row);
    }
} 

void addBallToCell(std::vector<std::vector<cell>> &grid, Ball ball){
    Vector2 max = Vector2{ball.position.x + ball.radius, ball.position.y + ball.radius};
    Vector2 min = Vector2{ball.position.x - ball.radius, ball.position.y - ball.radius};

    Vector2 indexAtMin = getNearestIndexAtPoint(min);
    Vector2 indexAtCenter = getNearestIndexAtPoint(ball.position);
    Vector2 indexAtMax = getNearestIndexAtPoint(max);
    Vector2 indexAtLowerRight = getNearestIndexAtPoint(Vector2{max.x, min.y}); 
    Vector2 indexAtUpperLeft = getNearestIndexAtPoint(Vector2{min.x, max.y}); 


    //std::cout << "Index At center" << indexAtCenter.x << " " << indexAtCenter.y << std::endl;
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

void updateCellContents(std::vector<std::vector<cell>> &grid, std::vector<Ball> &balls){
    for(int i = 0; i < grid.size(); i++){
        for(int j = 0; j < grid[i].size(); j++){
            grid[i][j].clearBalls();
            if(grid[i][j].ballsInCell.size() > 0){
                grid[i][j].color = BLUE;
            }
            else{
                grid[i][j].color = RED;
            }
        }
    }
    for(int k = 0; k < balls.size(); k++){
        addBallToCell(grid, balls[k]);
    }
}



void checkCollisionInCell(std::vector<std::vector<cell>> &grid, float elasticityCoefficient, std::vector<Ball> &ballArray){
    for(int i = 0; i < grid.size(); i++){
        for(int j = 0; j < grid[i].size(); j++){
            for(int k = 0; k < grid[i][j].ballsInCell.size(); k++){

                ballArray[grid[i][j].ballsInCell[k].index].position = Vector2Add(grid[i][j].ballsInCell[k].position, Vector2Scale(grid[i][j].ballsInCell[k].velocity, TIMESTEP));

                if (ballArray[grid[i][j].ballsInCell[k].index].position.x - ballArray[grid[i][j].ballsInCell[k].index].radius <= 0)
                {
                    ballArray[grid[i][j].ballsInCell[k].index].position.x = ballArray[grid[i][j].ballsInCell[k].index].radius;
                    ballArray[grid[i][j].ballsInCell[k].index].velocity.x *= -1;
                }
                if(ballArray[grid[i][j].ballsInCell[k].index].position.x + ballArray[grid[i][j].ballsInCell[k].index].radius >= WINDOW_WIDTH)
                {
                    ballArray[grid[i][j].ballsInCell[k].index].position.x = WINDOW_WIDTH - ballArray[grid[i][j].ballsInCell[k].index].radius;
                    ballArray[grid[i][j].ballsInCell[k].index].velocity.x *= -1;
                }
                if ( ballArray[grid[i][j].ballsInCell[k].index].position.y - ballArray[grid[i][j].ballsInCell[k].index].radius <= 0)
                {
                    ballArray[grid[i][j].ballsInCell[k].index].position.y = ballArray[grid[i][j].ballsInCell[k].index].radius;
                    ballArray[grid[i][j].ballsInCell[k].index].velocity.y *= -1;
                }
                if(ballArray[grid[i][j].ballsInCell[k].index].position.y + ballArray[grid[i][j].ballsInCell[k].index].radius >= WINDOW_HEIGHT)
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
                    if (isCirclesColliding(ballArray[grid[i][j].ballsInCell[k].index], ballArray[grid[i][j].ballsInCell[l].index]) && Vector2DotProduct(n, Vector2Subtract(grid[i][j].ballsInCell[k].velocity, grid[i][j].ballsInCell[l].velocity )) < 0)
                    {
                        float impulsej = -( ( 
                        (1 + elasticityCoefficient) * Vector2DotProduct(Vector2Subtract(ballArray[grid[i][j].ballsInCell[k].index].velocity, ballArray[grid[i][j].ballsInCell[l].index].velocity ), n)) 
                        / 
                        (Vector2DotProduct(n, n) * (1 / ballArray[grid[i][j].ballsInCell[k].index].mass) + (1 / ballArray[grid[i][j].ballsInCell[l].index].mass)
                        ));
                        
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

void InitializeBall(std::vector<Ball> &array, int arraySize, bool isLarge)
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
        array.push_back(ball);
    }
}

void updateBallsIndex(std::vector<Ball> &ballArray){
    for(int i = 0; i < ballArray.size(); i++){
        ballArray[i].index = i;
    }
}

Vector2 getCenterOfRectangle(Vector2 RectanglePos, float width, float height){ // ( (x1 + x2) / 2, (y1 + y2) / 2 )
    return Vector2{(RectanglePos.x + width)/2, (RectanglePos.y + height)/2};
}

int main()
{
    int elasticityCoefficient = 1.0f;

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "OlivaresTamano - Exercise 5");

    SetTargetFPS(FPS);

    float accumulator = 0;

    std::vector<Ball> ballArray;
    int spawnInstance = 0;
    
    std::vector<std::vector<cell>> grid;
    initializeAllCells(grid);
   
    bool drawGrid = false;
    while (!WindowShouldClose())
    {
        
        float delta_time = GetFrameTime();
        Vector2 forces = Vector2Zero();
        Vector2 mouseIndexLocation = getNearestIndexAtPoint(GetMousePosition());
        if(IsMouseButtonDown(0)){
            std::cout << "MOUSE INDEX: " << mouseIndexLocation.x << " " <<  mouseIndexLocation.y << std::endl;
            std::cout << "SIZE OF CELL: " << grid[mouseIndexLocation.y][mouseIndexLocation.x].ballsInCell.size() << std::endl;
        }

        updateBallsIndex(ballArray);
        updateCellContents(grid, ballArray);
        if (IsKeyPressed(KEY_TAB)){
            drawGrid = !drawGrid;
        }
        if (IsKeyPressed(KEY_SPACE))
        {
            if (spawnInstance == 10)
            {
                InitializeBall(ballArray, 1, true);
                spawnInstance = 0;
            }
            else
            {
                InitializeBall(ballArray, 25, false);
                spawnInstance++;
            }
        }
        
        // Physics
        accumulator += delta_time;
        while (accumulator >= TIMESTEP)
        {
            checkCollisionInCell(grid, elasticityCoefficient, ballArray);
            accumulator -= TIMESTEP;
        }
        const char* numberOfBalls = std::to_string(ballArray.size()).c_str();
        
        BeginDrawing();
        ClearBackground(WHITE);
        DrawText(numberOfBalls, 0, 0, 30, YELLOW);
        for (int i = 0; i < ballArray.size(); i++)
        {
            DrawCircleV(ballArray[i].position, ballArray[i].radius, ballArray[i].color);
        }

        if(drawGrid){
            for(int i = 0; i < grid.size(); i++){
                for(int j = 0; j < grid[i].size(); j++){
                    const char* numberOfBalsInCell = std::to_string(grid[i][j].ballsInCell.size()).c_str();
                    Vector2 rectMidpoint = Vector2{getCenterOfRectangle(grid[i][j].position, cellSize, cellSize)};
                    DrawText(numberOfBalsInCell, grid[i][j].max.x, grid[i][j].max.y, 5, PURPLE);
                    DrawRectangleLines(grid[i][j].position.x, grid[i][j].position.y, cellSize, cellSize, grid[i][j].color);
                }
            }
        }


        EndDrawing();
    }
    CloseWindow();
    return 0;
}