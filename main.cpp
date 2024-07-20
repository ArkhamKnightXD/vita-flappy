#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <iostream>
#include <fstream>

const int SCREEN_WIDTH = 960;
const int SCREEN_HEIGHT = 544;

SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;
SDL_GameController* controller = nullptr;

bool isGameOver;
bool isGamePaused;
float startGameTimer;

bool shouldRotateUp = false;
float downRotationTimer = 0;
float upRotationTimer = 0;

float gravity = 0;

Mix_Chunk *gamePausedSound = nullptr;
Mix_Chunk *flapSound = nullptr;
Mix_Chunk *pauseSound = nullptr;
Mix_Chunk *dieSound = nullptr;
Mix_Chunk *crossPipeSound = nullptr;

typedef struct
{
    SDL_Texture *texture;
    SDL_Rect textureBounds;
} Sprite;

SDL_Rect birdsBounds;
Sprite birdSprites;
Sprite playerSprite;
Sprite startGameSprite;
Sprite backgroundSprite;
Sprite groundSprite;

Sprite upPipeSprite;
Sprite downPipeSprite;

std::vector<Sprite> numbers;
std::vector<Sprite> numberTens;
std::vector<Sprite> highScoreNumbers;
std::vector<Sprite> highScoreNumberTens;

typedef struct
{
    float y;
    Sprite sprite;
    float impulse;
    float gravityIncrement;
} Player;

Player player;

float groundYPosition;

SDL_Rect groundCollisionBounds;

SDL_Texture *highScoreTexture = nullptr;
SDL_Rect highScoreBounds;

TTF_Font *fontSquare = nullptr;

SDL_Color fontColor = {255, 255, 255};

int score = 0;
float initialAngle = 0;
int highScore;

typedef struct
{
    float x;
    float y;
} Vector2;

std::vector<Vector2> groundPositions;

typedef struct
{
    float x;
    Sprite sprite;
    bool isBehind;
    bool isDestroyed;
} Pipe;

std::vector<Pipe> pipes;

float lastPipeSpawnTime;

void generatePipes()
{
    int upPipePosition = rand() % 220;

    upPipePosition *= -1;

    SDL_Rect upPipeBounds = {SCREEN_WIDTH, upPipePosition, upPipeSprite.textureBounds.w, upPipeSprite.textureBounds.h};

    Sprite upSprite = {upPipeSprite.texture, upPipeBounds};

    Pipe upPipe = {SCREEN_WIDTH, upSprite, false, false};

    // gap size = 80.
    int downPipePosition = upPipePosition + upPipeSprite.textureBounds.h + 80;

    SDL_Rect downPipeBounds = {SCREEN_WIDTH, downPipePosition, downPipeSprite.textureBounds.w, downPipeSprite.textureBounds.h};

    Sprite downSprite = {downPipeSprite.texture, downPipeBounds};

    Pipe downPipe = {SCREEN_WIDTH, downSprite, false, false};

    pipes.push_back(upPipe);
    pipes.push_back(downPipe);

    lastPipeSpawnTime = 0;
}

int loadHighScore()
{
    std::string highScoreText;

    std::ifstream highScores("high-score.txt");

    getline(highScores, highScoreText);

    highScores.close();

    int highScore = stoi(highScoreText);

    return highScore;
}

//Save score not working in original hardware.
void saveScore()
{
    std::ofstream highScores("high-score.txt");

    std::string scoreString = std::to_string(score);
    highScores << scoreString;

    highScores.close();
}

void resetGame(Player &player)
{
    if (score > highScore)
    {
        saveScore();
    }

    highScore = loadHighScore();

    isGameOver = false;
    startGameTimer = 0;
    score = 0;
    startGameTimer = 0;
    initialAngle = 0;
    player.y = SCREEN_HEIGHT / 2;
    player.sprite.textureBounds.x = SCREEN_WIDTH / 2;
    player.sprite.textureBounds.y = SCREEN_HEIGHT / 2;
    gravity = 0;
    pipes.clear();
}

void quitGame()
{
    Mix_FreeChunk(flapSound);
    SDL_DestroyTexture(playerSprite.texture);
    SDL_DestroyTexture(highScoreTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

void handleEvents(float deltaTime)
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {

        if (event.type == SDL_QUIT) {
                
            quitGame();
            exit(0);
        }

        if (event.type == SDL_CONTROLLERBUTTONDOWN && event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
        {
            isGamePaused = !isGamePaused;
            Mix_PlayChannel(-1, gamePausedSound, 0);
        }

        if (!isGameOver && event.type == SDL_CONTROLLERBUTTONDOWN && event.cbutton.button == SDL_CONTROLLER_BUTTON_A)
        {
            gravity = player.impulse * deltaTime;

            shouldRotateUp = true;
            upRotationTimer = 1;
            downRotationTimer = 0;
            initialAngle = -20;

            Mix_PlayChannel(-1, flapSound, 0);
        }

        else if (isGameOver && event.type == SDL_CONTROLLERBUTTONDOWN && event.cbutton.button == SDL_CONTROLLER_BUTTON_A)
        {
            resetGame(player);
        }
    }
}

void updateTextureText(SDL_Texture *&texture, const char *text)
{
    if (fontSquare == nullptr)
    {
        printf("TTF_OpenFont fontSquare: %s\n", TTF_GetError());
    }

    SDL_Surface *surface = TTF_RenderUTF8_Blended(fontSquare, text, fontColor);
    if (surface == nullptr)
    {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to create text surface! SDL Error: %s\n", SDL_GetError());
        exit(3);
    }

    SDL_DestroyTexture(texture);
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == nullptr)
    {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to create texture from surface! SDL Error: %s\n", SDL_GetError());
    }

    SDL_FreeSurface(surface);
}

Sprite loadSprite(const char *file, int positionX, int positionY)
{
    SDL_Rect textureBounds = {positionX, positionY, 0, 0};

    SDL_Texture *texture = IMG_LoadTexture(renderer, file);

    if (texture != nullptr)
    {
        SDL_QueryTexture(texture, NULL, NULL, &textureBounds.w, &textureBounds.h);
    }

    Sprite sprite = {texture, textureBounds};

    return sprite;
}

Mix_Chunk *loadSound(const char *p_filePath)
{
    Mix_Chunk *sound = nullptr;

    sound = Mix_LoadWAV(p_filePath);
    if (sound == nullptr)
    {
        printf("Failed to load scratch sound effect! SDL_mixer Error: %s\n", Mix_GetError());
    }

    return sound;
}

void update(float deltaTime)
{
    startGameTimer += deltaTime;

    lastPipeSpawnTime += deltaTime;

    if (lastPipeSpawnTime >= 2)
    {
        generatePipes();
    }

    if (player.y < -player.sprite.textureBounds.h)
    {
        isGameOver = true;
    }
    
    if (startGameTimer > 1)
    {
        player.y += gravity * deltaTime;
        player.sprite.textureBounds.y = player.y;
        gravity += player.gravityIncrement * deltaTime;
    }

    if (SDL_HasIntersection(&player.sprite.textureBounds, &groundCollisionBounds))
    {
        isGameOver = true;
        Mix_PlayChannel(-1, dieSound, 0);
    }

    for (Vector2 &groundPosition : groundPositions)
    {
        groundPosition.x -= 150 * deltaTime;

        if (groundPosition.x < -groundSprite.textureBounds.w)
        {
            groundPosition.x = groundSprite.textureBounds.w * 3;
        }
    }

    for (auto actualPipe = pipes.begin(); actualPipe != pipes.end();)
    {
        if (!actualPipe->isDestroyed)
        {
            actualPipe->x -= 150 * deltaTime;
            actualPipe->sprite.textureBounds.x = actualPipe->x;
        }

        if (SDL_HasIntersection(&player.sprite.textureBounds, &actualPipe->sprite.textureBounds))
        {
            isGameOver = true;
            Mix_PlayChannel(-1, dieSound, 0);
        }

        if (!actualPipe->isBehind && player.sprite.textureBounds.x > actualPipe->sprite.textureBounds.x)
        {
            actualPipe->isBehind = true;

            if (actualPipe->sprite.textureBounds.y < player.sprite.textureBounds.y)
            {
                score++;
                Mix_PlayChannel(-1, crossPipeSound, 0);
            }
        }

        if (actualPipe->sprite.textureBounds.x < -actualPipe->sprite.textureBounds.w)
        {
            actualPipe->isDestroyed = true;
            pipes.erase(actualPipe);
        }
        else
        {
            actualPipe++;
        }
    }
}

void renderSprite(Sprite sprite)
{
    SDL_RenderCopy(renderer, sprite.texture, NULL, &sprite.textureBounds);
}

void render(float deltaTime)
{
    backgroundSprite.textureBounds.x = 0;
    renderSprite(backgroundSprite);

    backgroundSprite.textureBounds.x = backgroundSprite.textureBounds.w;
    renderSprite(backgroundSprite);

    backgroundSprite.textureBounds.x = backgroundSprite.textureBounds.w * 2;
    renderSprite(backgroundSprite);

    backgroundSprite.textureBounds.x = backgroundSprite.textureBounds.w * 3;
    renderSprite(backgroundSprite);

    groundSprite.textureBounds.x = 0;
    renderSprite(groundSprite);

    groundSprite.textureBounds.x = groundSprite.textureBounds.w;
    renderSprite(groundSprite);

    groundSprite.textureBounds.x = groundSprite.textureBounds.w * 2;
    renderSprite(groundSprite);

    groundSprite.textureBounds.x = groundSprite.textureBounds.w * 3;
    renderSprite(groundSprite);

    for (Pipe pipe : pipes)
    {
        if (!pipe.isDestroyed)
        {
            renderSprite(pipe.sprite);
        }
    }

    if (highScore < 10)
    {
        highScoreNumbers[highScore].textureBounds.x = 320;
        renderSprite(highScoreNumbers[highScore]);
    }
    else
    {
        int tens = (int)(highScore / 10);
        int units = (int)(highScore % 10);

        highScoreNumberTens[tens].textureBounds.x = 300;
        highScoreNumbers[units].textureBounds.x = 320;

        renderSprite(highScoreNumberTens[tens]);
        renderSprite(highScoreNumbers[units]);
    }

    if (score < 10)
    {
        renderSprite(numbers[score]);
    }
    else
    {
        int tens = (int)(score / 10);
        int units = (score % 10);

        numberTens[tens].textureBounds.x = SCREEN_WIDTH / 2 - 20;

        renderSprite(numberTens[tens]);
        renderSprite(numbers[units]);
    }

    SDL_RenderCopy(renderer, highScoreTexture, NULL, &highScoreBounds);

    for (Vector2 groundPosition : groundPositions)
    {
        groundSprite.textureBounds.x = groundPosition.x;
        renderSprite(groundSprite);
    }

    if (isGameOver)
    {
        renderSprite(startGameSprite);
    }

    SDL_RenderCopyEx(renderer, birdSprites.texture, &birdsBounds, &player.sprite.textureBounds, initialAngle, NULL, SDL_FLIP_NONE);

    if (startGameTimer > 1)
    {
        downRotationTimer += deltaTime;

        if (downRotationTimer < 0.5f)
        {
            SDL_RenderCopyEx(renderer, birdSprites.texture, &birdsBounds, &player.sprite.textureBounds, initialAngle, NULL, SDL_FLIP_NONE);
        }

        if (shouldRotateUp)
        {
            if (upRotationTimer > 0)
            {
                upRotationTimer -= deltaTime;
            }

            if (upRotationTimer <= 0)
            {
                shouldRotateUp = false;
            }

            SDL_RenderCopyEx(renderer, birdSprites.texture, &birdsBounds, &player.sprite.textureBounds, initialAngle, NULL, SDL_FLIP_NONE);
        }

        if (downRotationTimer > 0.5f)
        {
            if (initialAngle <= 90 && !isGameOver && !isGamePaused)
            {
                initialAngle += 2;
            }

            SDL_RenderCopyEx(renderer, birdSprites.texture, &birdsBounds, &player.sprite.textureBounds, initialAngle, NULL, SDL_FLIP_NONE);
        }
    }
    else
    {
        SDL_RenderCopyEx(renderer, birdSprites.texture, &birdsBounds, &player.sprite.textureBounds, initialAngle, NULL, SDL_FLIP_NONE);
    }

    SDL_RenderPresent(renderer);
}

void loadNumbersSprites()
{
    std::string fileExtension = ".png";

    for (int i = 0; i < 10; i++)
    {
        std::string completeString = std::to_string(i) + fileExtension;

        Sprite numberSprite = loadSprite(completeString.c_str(), SCREEN_WIDTH / 2, 30);

        numbers.push_back(numberSprite);
        numberTens.push_back(numberSprite);

        highScoreNumbers.push_back(numberSprite);
        highScoreNumberTens.push_back(numberSprite);
    }
}

int main(int argc, char *args[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0)
    {
        std::cout << "SDL crashed. Error: " << SDL_GetError();
        return 1;
    }

    window = SDL_CreateWindow("My Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr)
    {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (SDL_NumJoysticks() < 1) {
        printf("No game controllers connected!\n");
        return -1;
    } 
    else {

        controller = SDL_GameControllerOpen(0);
        if (controller == NULL) {

            printf("Unable to open game controller! SDL Error: %s\n", SDL_GetError());
            return -1;
        }
    }

    if (!IMG_Init(IMG_INIT_PNG))
    {
        std::cout << "SDL_image crashed. Error: " << SDL_GetError();
        return 1;
    }

    // Initialize SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
    }

    if (TTF_Init() == -1)
    {
        return 1;
    }

    fontSquare = TTF_OpenFont("square_sans_serif_7.ttf", 36);

    updateTextureText(highScoreTexture, "High Score: ");

    SDL_QueryTexture(highScoreTexture, NULL, NULL, &highScoreBounds.w, &highScoreBounds.h);
    highScoreBounds.x = 20;
    highScoreBounds.y = 30;

    gamePausedSound = loadSound("magic.wav");
    flapSound = loadSound("wing.wav");
    pauseSound = loadSound("magic.wav");
    dieSound = loadSound("die.wav");
    crossPipeSound = loadSound("point.wav");

    highScore = loadHighScore();

    upPipeSprite = loadSprite("pipe-green-180.png", SCREEN_WIDTH / 2, -220);
    downPipeSprite = loadSprite("pipe-green.png", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

    startGameSprite = loadSprite("message.png", SCREEN_WIDTH / 2 - 75, 103);
    backgroundSprite = loadSprite("background-day.png", 0, 0);

    groundSprite = loadSprite("base.png", 0, 0);

    groundYPosition = SCREEN_HEIGHT - groundSprite.textureBounds.h;

    groundSprite.textureBounds.y = groundYPosition;

    groundCollisionBounds = {0, (int)groundYPosition, SCREEN_HEIGHT, groundSprite.textureBounds.h};

    groundPositions.push_back({0, groundYPosition});
    groundPositions.push_back({(float)groundSprite.textureBounds.w, groundYPosition});
    groundPositions.push_back({(float)groundSprite.textureBounds.w * 2, groundYPosition});
    groundPositions.push_back({(float)groundSprite.textureBounds.w * 3, groundYPosition});

    playerSprite = loadSprite("yellowbird-midflap.png", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

    player = Player{SCREEN_HEIGHT / 2, playerSprite, -10000, 400};

    loadNumbersSprites();

    birdSprites = loadSprite("yellow-bird.png", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

    birdsBounds = {0, 0, birdSprites.textureBounds.w / 3, birdSprites.textureBounds.h};

    int framesCounter = 0;
    int framesSpeed = 6;

    int currentFrame = 0;

    Uint32 previousFrameTime = SDL_GetTicks();
    Uint32 currentFrameTime = previousFrameTime;
    float deltaTime = 0.0f;

    srand(time(NULL));

    while (true)
    {
        currentFrameTime = SDL_GetTicks();

        deltaTime = (currentFrameTime - previousFrameTime) / 1000.0f;

        previousFrameTime = currentFrameTime;

        handleEvents(deltaTime);

        if (!isGameOver && !isGamePaused)
        {

            framesCounter++;

            if (framesCounter >= (60 / framesSpeed))
            {
                framesCounter = 0;
                currentFrame++;

                if (currentFrame > 2)
                    currentFrame = 0;

                birdsBounds.x = currentFrame * birdsBounds.w;
            }
            
            update(deltaTime);
        }

        render(deltaTime);
    }
}