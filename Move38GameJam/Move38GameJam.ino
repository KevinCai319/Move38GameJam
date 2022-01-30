#define GREEN makeColorRGB(0,255,0)
#define CYAN makeColorRGB(20,205,205)
#define PINK makeColorRGB(255,105,180)
#define FAIL_COLOR 254
#define SPINNER_SPIN_TIME 1500
//Success color would just be team color to make things easier.

//List of messages that can be sent.
#define SPINNER_SET_MSG 8
#define RESET 9
#define RANDOMIZE 10
#define RESET_ROUND 11
//color range
#define DESIRED_COLOR_START 50
#define CHECK_TYPE 12
#define IM_FOLLOWER 13
#define IM_SPINNER 14
#define IM_PLAYER 15
#define PLAYER_WIN 16
#define PLAYER_LOSS 17
#define PLAYER_OFF 18
#define GAME_END 19
#define PLAYER_UPDATE_BLINK_RATE 300

#define DESIRED_COLOR_END DESIRED_COLOR_START+NUM_COLORS
#define NUM_COLORS 6
//List of colors that can appear in-game.
const Color COLORS[] = {RED, GREEN, BLUE, YELLOW, MAGENTA, WHITE,ORANGE,CYAN};


//Quick way to classify tiles.
enum Tile {
  SPINNER,
  FOLLOWER,
  PLAYER,
  UNASSIGNED
};


Tile tile_type = UNASSIGNED;
//Usage as temporary time variable.
long last_time = 0;

//Usage to manage states.
//TODO: convert all the bools into a byte to reduce memory usage.
bool game_running = false;
byte current_color = 0;
byte tiles_in_network = 0;
byte lives = 6;
bool player_finalized = false;
bool listen_for_player = false;
byte face_colors[6] = {0, 0, 0, 0, 0, 0};
byte face_connected[6] = {0, 0 , 0 , 0, 0, 0};
bool face_changed[6] = {0, 0 , 0 , 0, 0, 0};
bool spinning = false;
byte spins_done = 0;
bool timer_started = false;


//Follower-related variables
bool follower_disabled = false;
bool game_over = false;
void setup() {
  //set seed for RNG.
  randomize();
  //Every single tile changes to a random color.
  create_random_colors();
}
//This area meant for followers and uninitialized tiles.
void create_random_colors() {
  FOREACH_FACE(f)color_face(random(NUM_COLORS - 1), f);
}
void color_face(byte c, byte f) {
  if (c == FAIL_COLOR) {
    setColorOnFace(OFF, f);
  }
  setColorOnFace(COLORS[c], f);
  face_colors[f] = c;
}
void color_full(byte c) {
  FOREACH_FACE(f) {
    color_face(c, f);
  }
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
  timer_started = false;
  FOREACH_FACE(f) {
    color_face(f, f);
  }
  spinning = true;
  spins_done = 0;
}

void reset_game() {
  tile_type = UNASSIGNED;
  setValueSentOnAllFaces(RESET);
  game_running = false;
  game_over = true;
  listen_for_player = false;
  player_finalized = false;
  timer_started = false;
}

//PLAYER CODE

//PLAYER HELPER FUNCTIONS

//Set the player's 4 panels that represent team color.
void set_player_team_color() {
  for (int i = 0; i < lives; i++) {
    color_face(current_color, i);
  }
  for (int i = lives; i < 6; i++) {
    color_face(FAIL_COLOR, i);
  }
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
      if(current_color == 8){
        current_color = 2;
      }
      set_player_team_color();
    }
  }
}

//PLAYER MAIN FUNCTIONS
void update_player() {
  if (!player_finalized) {
    player_setup();
  } else {
    FOREACH_FACE(f) {
      if (didValueOnFaceChange(f) && getLastValueReceivedOnFace(f) == RESET && face_connected[f]) {
        reset_game();
      } else if (face_changed[f] && getLastValueReceivedOnFace(f) == PLAYER_WIN && face_connected[f]) {
        color_full(1);
        timer_started = true;
        last_time = millis();
      } else if (face_changed[f] && getLastValueReceivedOnFace(f) == PLAYER_LOSS && face_connected[f]) {
        if (!timer_started) {
          color_full(0);
          timer_started = true;
          last_time = millis();
          if (lives > 0)lives--;
        }
      } else {

      }
    }
    if (timer_started && millis() - last_time > PLAYER_UPDATE_BLINK_RATE) {
      set_player_team_color();
      timer_started = false;
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
        //set color to white.
        color_full(5);

        if (buttonDoubleClicked()) {
          tile_type = PLAYER;
          player_finalized = false;
          lives = 6;
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
            game_running = true;
          }
        }
      }
      break;
    case PLAYER:
      update_player();
      break;
    case SPINNER:
      if (spinning) {
        if (millis() - last_time >= 250) {
          rotate_colors();
          last_time = millis();
          spins_done++;
          if (spins_done == 1) {
            current_color = random(NUM_COLORS - 1);
            setValueSentOnAllFaces(DESIRED_COLOR_START + current_color);
          }
          if (spins_done == 6) {
            spinning = false;
            color_full(current_color);
            setValueSentOnAllFaces(RANDOMIZE);
            listen_for_player = true;
          }
        }
      } else {
        if (!timer_started && listen_for_player) {
          timer_started = true;
          last_time = millis();
          spins_done = 0;
        } else {
          if (listen_for_player) {
            if (millis() - last_time >= 500) {
              last_time = millis();
              color_face(FAIL_COLOR, spins_done);
              spins_done++;
              if (spins_done >= 6) {
                timer_started = false;
                listen_for_player = false;
                setValueSentOnAllFaces(GAME_END);
              }
            }
          } else {
            if (buttonSingleClicked()) {
              reset_game();
            }
          }
        }

      }
      break;
    case FOLLOWER:
      FOREACH_FACE(f) {
        //Did the value on a face change?
        if (didValueOnFaceChange(f)) {
          if (getLastValueReceivedOnFace(f) == RESET) {
            setValueSentOnAllFaces(RESET);
            tile_type = UNASSIGNED;
            game_running = false;
            listen_for_player = false;
            game_over = false;
          } else if (getLastValueReceivedOnFace(f) == RANDOMIZE) {
            create_random_colors();
            follower_disabled = false;
            game_running = true;
            game_over = false;
            listen_for_player = true;
            setValueSentOnAllFaces(RANDOMIZE);
          } else if (getLastValueReceivedOnFace(f) >= DESIRED_COLOR_START && getLastValueReceivedOnFace(f) < DESIRED_COLOR_END) {
            current_color = getLastValueReceivedOnFace(f) - DESIRED_COLOR_START;
            setValueSentOnAllFaces(getLastValueReceivedOnFace(f));
            //color_full(current_color);
          } else if (getLastValueReceivedOnFace(f) == GAME_END) {
            listen_for_player = false;
            game_over = true;
            FOREACH_FACE(g) {
              if (getLastValueReceivedOnFace(g) != IM_PLAYER) {
                setValueSentOnFace(getLastValueReceivedOnFace(f), g);
              }
            }
          } else if (getLastValueReceivedOnFace(f) == IM_PLAYER && game_over) {
            setValueSentOnFace(PLAYER_LOSS, f);
          } else {

          }
        } else {
          if (getLastValueReceivedOnFace(f) == IM_PLAYER && !isValueReceivedOnFaceExpired(f)) {
            if (buttonSingleClicked() && !game_over && listen_for_player) {
              if (face_colors[f] == current_color) {
                setValueSentOnFace(PLAYER_WIN, f);
                listen_for_player = false;
                follower_disabled = true;
                color_full(FAIL_COLOR);
              } else {
                setValueSentOnFace(PLAYER_LOSS, f);
              }
            } else {
              if (game_over && face_changed[f]) {
                setValueSentOnFace(PLAYER_LOSS, f);
              }
            }
          }
        }
      }
      if(game_over && follower_disabled){
        if(millis() % 1000 > 500){
          color_full(FAIL_COLOR);
        }else{
          color_full(current_color);
        }
      }

      break;
    default:
      break;
  }
  buttonSingleClicked();
  buttonDoubleClicked();
  buttonLongPressed();
}
