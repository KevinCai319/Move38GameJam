#define GREEN makeColorRGB(0,255,0)
#define CYAN makeColorRGB(20,205,205)
#define FAIL_COLOR 254
#define SPINNER_SPIN_TIME 1500
//Success color would just be team color to make things easier.

//List of messages that can be sent.
#define SPINNER_SET_MSG 8
#define RESET 9
#define RANDOMIZE 10
#define GAME_END 19
//color range
#define DESIRED_COLOR_START 50

#define IM_PLAYER 15
#define PLAYER_WIN 16
#define PLAYER_LOSS 17

#define PLAYER_UPDATE_BLINK_RATE 300

#define DESIRED_COLOR_END DESIRED_COLOR_START+NUM_COLORS
#define NUM_COLORS 6
//List of colors that can appear in-game.
const Color COLORS[] = {RED, GREEN, BLUE, YELLOW, MAGENTA, WHITE, ORANGE, CYAN};

enum Tile {
  SPINNER,
  FOLLOWER,
  PLAYER,
  UNASSIGNED
};

enum GameState {
  SETUP,
  SPINNING,
  PLAYING,
  END
};


Tile tile_type = UNASSIGNED;
GameState state = SETUP;
//Usage as temporary time variable. Used for all states.
long last_time = 0;

//Usage to manage states.
byte current_color = 0;

//Used more like a counter variable (Used to count spinner spins, spinner countdown, player lives)
byte lives = 6;

//Used to keep track of face colors and connections.
byte face_colors[6] = {0, 0, 0, 0, 0, 0};
byte face_connected[6] = {0, 0 , 0 , 0, 0, 0};
bool face_changed[6] = {0, 0 , 0 , 0, 0, 0};

//Player-related variables
bool player_timer_started = false;
bool player_finalized = false;

//Follower-related variables
bool follower_disabled = false;

void setup() {
  randomize();
  create_random_colors();
}
//This area meant for all tiles to use.
void create_random_colors() {
  FOREACH_FACE(f)color_face(random(NUM_COLORS - 1), f);
}

void color_face(byte c, byte f) {
  //special colors not defined in array.
  if (c == FAIL_COLOR) {
    setColorOnFace(OFF, f);
  }else{
    setColorOnFace(COLORS[c], f);
  }
  face_colors[f] = c;
}

void color_full(byte c) {
  FOREACH_FACE(f) {
    color_face(c, f);
  }
}

void reset_game() {
  tile_type = UNASSIGNED;
  state = SETUP;
  setValueSentOnAllFaces(RESET);
  player_finalized = false;
  player_timer_started = false;
  follower_disabled = false;
}


//This area meant for spinners.
void initialize_game() {
  tile_type = SPINNER;
  setValueSentOnAllFaces(SPINNER_SET_MSG);
  spinner_spin();
}

void rotate_colors() {
  byte tmp = face_colors[0];
  FOREACH_FACE(f) {
    color_face(face_colors[(f + 1) % 6], f);
  }
  color_face(tmp, 5);
}

void spinner_spin() {
  last_time = millis();
  FOREACH_FACE(f)color_face(f, f);
  state = SPINNING;
  lives = 0;
}

//PLAYER CODE

//PLAYER HELPER FUNCTIONS

//Set the player's 4 panels that represent team color.
void set_player_team_color() {
  for (int i = 0; i < 6; i++)color_face((i < lives) ? current_color : FAIL_COLOR, i);
}

void player_setup() {
  if (!isAlone()) {
    //Accidentally added one too many players? Throw them back into the game.
    tile_type = UNASSIGNED;
  } else {
    if (buttonDoubleClicked()) {
      player_finalized = true;
      lives = 6;
    } else if (buttonSingleClicked()) {
      current_color++;
      if (current_color == 8) {
        current_color = 2;
      }
      set_player_team_color();
    }
  }
}

//PLAYER MAIN FUNCTION
void update_player() {
  if (!player_finalized) {
    player_setup();
  } else {
    FOREACH_FACE(f) {
      if (face_connected[f]) {
        switch (getLastValueReceivedOnFace(f)) {
          case RESET:
            if (didValueOnFaceChange(f)) reset_game();
            break;
          case PLAYER_WIN:
            if (!face_changed[f] || player_timer_started) break;
            player_timer_started = true;
            last_time = millis();
            color_full(1);
            break;
          case PLAYER_LOSS:
            if (!face_changed[f] || player_timer_started) break;
            player_timer_started = true;
            last_time = millis();
            color_full(0);
            if (lives > 0)lives--;
            break;
        }
      }
    }
    if (player_timer_started && millis() - last_time > PLAYER_UPDATE_BLINK_RATE) {
      set_player_team_color();
      player_timer_started = false;
    }
    setValueSentOnAllFaces(IM_PLAYER);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  FOREACH_FACE(f) {
    byte tmp = face_connected[f];
    if (isValueReceivedOnFaceExpired(f)) {
      face_connected[f] = 0;
    } else {
      face_connected[f] = getLastValueReceivedOnFace(f);
    }
    face_changed[f] = face_connected[f] != tmp;
  }
  switch (tile_type) {
    case UNASSIGNED:
      if (isAlone()) {
        //set color to blink white.
        color_full((millis()%700 > 350)?5:FAIL_COLOR);
        if (buttonDoubleClicked()) {
          tile_type = PLAYER;
          player_finalized = false;
          lives = 6;
          current_color = 2;
          set_player_team_color();
          break;
        }
      } else {
        if (millis() - last_time >= 250) {
          create_random_colors();
          last_time = millis();
        }
        if (buttonSingleClicked()) {
          initialize_game();
          break;
        }
        FOREACH_FACE(f) {
          if (didValueOnFaceChange(f) && getLastValueReceivedOnFace(f) == SPINNER_SET_MSG) {
            color_full(5);//set to white;
            setValueSentOnAllFaces(SPINNER_SET_MSG);
            tile_type = FOLLOWER;
          }
        }
      }
      break;
    case PLAYER:
      update_player();
      break;
    case SPINNER:
      switch (state) {
        case SPINNING:
          if (millis() - last_time >= 250) {
            rotate_colors();
            last_time = millis();
            lives++;
            if (lives == 1) {
              current_color = random(NUM_COLORS - 1);
              setValueSentOnAllFaces(DESIRED_COLOR_START + current_color);
            }else if (lives == 6) {
              color_full(current_color);
              setValueSentOnAllFaces(RANDOMIZE);
              last_time = millis();
              state = PLAYING;
            }
          }
          break;
        case PLAYING:
          if (millis() - last_time >= 500) {
            last_time = millis();
            lives--;
            set_player_team_color();
            if (lives == 0) {
              lives = 6;
              setValueSentOnAllFaces(GAME_END);
              state = END;
            }
          }
          break;
        case END:
          if (buttonSingleClicked()) {
            reset_game();
          }
          break;
        default:
          break;
      }
      break;
    case FOLLOWER:
      FOREACH_FACE(f) {
        if (didValueOnFaceChange(f)) {
          if (getLastValueReceivedOnFace(f) == RESET ) {
            reset_game();
          } else if (getLastValueReceivedOnFace(f) == RANDOMIZE) {
            create_random_colors();
            state = PLAYING;
            follower_disabled = false;
            setValueSentOnAllFaces(RANDOMIZE);
          } else if (getLastValueReceivedOnFace(f) >= DESIRED_COLOR_START && getLastValueReceivedOnFace(f) < DESIRED_COLOR_END) {
            state = SPINNING;
            current_color = getLastValueReceivedOnFace(f) - DESIRED_COLOR_START;
            setValueSentOnAllFaces(getLastValueReceivedOnFace(f));
          } else if (getLastValueReceivedOnFace(f) == GAME_END) {
            state = END;
            FOREACH_FACE(g) {
              if (getLastValueReceivedOnFace(g) != IM_PLAYER) {
                setValueSentOnFace(getLastValueReceivedOnFace(f), g);
              }
            }
          } else if (getLastValueReceivedOnFace(f) == IM_PLAYER && state == END) {
            setValueSentOnFace(PLAYER_LOSS, f);
          }
        } else {
          if (getLastValueReceivedOnFace(f) == IM_PLAYER && face_connected[f]) {
            if (buttonSingleClicked() && state == PLAYING) {
              if (face_colors[f] == current_color && !follower_disabled) {
                setValueSentOnFace(PLAYER_WIN, f);
                follower_disabled = true;
                color_full(FAIL_COLOR);
              } else {
                setValueSentOnFace(PLAYER_LOSS, f);
              }
            } else if (state == END && face_changed[f]){
              setValueSentOnFace(PLAYER_LOSS, f);
            }
          }
        }
      }
      if (state == END && follower_disabled){
        color_full((millis() % 1000 > 500)?FAIL_COLOR:current_color);
      }
      break;
    default:
      break;
  }
  buttonSingleClicked();
  buttonDoubleClicked();
}
