#define GREEN makeColorRGB(0,255,0)
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

#define DESIRED_COLOR_END DESIRED_COLOR_START+NUM_COLORS
#define NUM_COLORS 6
//List of colors that can appear in-game.
const Color COLORS[] = {RED,GREEN,BLUE,YELLOW,MAGENTA,WHITE};

//Quick way to classify tiles.
enum Tile{
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
byte face_colors[6] = {0,0,0,0,0,0};
bool spinning = false;
byte spins_done = 0;
bool timer_started = false;
bool follower_disabled = false;
void setup() {
  //set seed for RNG.
  randomize();
  //Every single tile changes to a random color.
  create_random_colors();
}
//This area meant for followers and uninitialized tiles.
void create_random_colors(){
  FOREACH_FACE(f)color_face(random(NUM_COLORS-1),f);
}
void color_face(byte c, byte f){
  if(c == FAIL_COLOR){
    setColorOnFace(OFF,f);
  }
  setColorOnFace(COLORS[c],f);
  face_colors[f] = c;
}
void color_full(byte c){
  FOREACH_FACE(f){
    color_face(c,f);
  }
}


//This area meant for spinners.
void initialize_game(){
  tile_type = SPINNER;
  setValueSentOnAllFaces(SPINNER_SET_MSG);
  spinner_spin();
}
void rotate_colors(){
  byte tmp = face_colors[0];
  FOREACH_FACE(f){
    color_face(face_colors[(f+1)%6],f);
  }
  color_face(tmp,5);
}

void spinner_spin(){
  last_time = millis();
  timer_started = false;
  //Setup the spinner's colors.
  FOREACH_FACE(f){
    color_face(f,f);
  }
  spinning = true;
  spins_done = 0;
}
void reset_game(){
  tile_type = UNASSIGNED;
  setValueSentOnAllFaces(RESET);
  game_running = false;
}
//This area meant for players.

//TODO: Create a way to finalize player setup.
void player_setup(){
   if(!isAlone()){
      //Accidentally added one too many players? Throw them back into the game.
      tile_type = UNASSIGNED;
    }else{
      if(buttonSingleClicked()){
        current_color++;
        current_color%=NUM_COLORS;
        set_player_team_color();
      }
      if(buttonDoubleClicked()){
        player_finalized=true;
      }
    }
}
//Set the player's 4 panels that represent team color.
void set_player_team_color(){
  for(int i = 0;i < lives;i++){
    color_face(current_color,i);
  }
  for(int i = lives;i<6;i++){
    color_face(FAIL_COLOR,i);
  }
}
void set_player_status(bool isConnected){
  
}

void loop() {
  // put your main code here, to run repeatedly:
  switch(tile_type){
    case UNASSIGNED:
      if(isAlone()){
        //Some user feedback that this device is not connected to anything.
        color_full(5);
        
        if(buttonDoubleClicked()){
          tile_type = PLAYER;
          player_finalized = false;
          color_face(FAIL_COLOR,0);
          color_face(FAIL_COLOR,1);
          set_player_team_color();
        }
      }else{
        if(millis()-last_time >= 250){
          create_random_colors();
          last_time = millis();
        }
        if(buttonSingleClicked()){
          initialize_game();
          break;
        }
        FOREACH_FACE(f) {
          if(didValueOnFaceChange(f) && getLastValueReceivedOnFace(f) == SPINNER_SET_MSG){
            color_full(5);//set to white;
            setValueSentOnAllFaces(SPINNER_SET_MSG);
            tile_type = FOLLOWER;
            game_running = true;
          }
        }
        
 
      }
    break;
    case PLAYER:
      if(!player_finalized){
        lives = 6;
        player_setup();
      }else{
        //TODO: Have the player read in data from sides if any follower tells it to do something.
        FOREACH_FACE(f) {
            if(didValueOnFaceChange(f)){
//              if(getLastValueReceivedOnFace(f) == RESET){
//                tile_type = UNASSIGNED;
//                player_finalized = false;
//              }
               if(getLastValueReceivedOnFace(f) == PLAYER_WIN){
                 color_full(1);
                 timer_started = true;
                 last_time = millis();
               }
               if(getLastValueReceivedOnFace(f) == PLAYER_LOSS){
                  color_full(0);
                  timer_started=true;
                  last_time = millis();
                  if(lives >0)lives--;
               }
               if(getLastValueReceivedOnFace(f) == PLAYER_OFF){
                
               }
            }
        }
        if(timer_started && millis() - last_time > 300){
           set_player_team_color();
           timer_started = false;
        }
        setValueSentOnAllFaces(IM_PLAYER);
      }
    break;
    case SPINNER:
      //if there is a triple click, just reset everything so everything is OK.
      if(buttonMultiClicked()){
         reset_game();
      }else{
        if(spinning){
         
          if(millis()-last_time >= 250){
            rotate_colors();
            last_time = millis();
            spins_done++;
            if(spins_done == 1){
              current_color = random(NUM_COLORS-1);
              setValueSentOnAllFaces(DESIRED_COLOR_START+current_color);
            }
            if(spins_done == 6){
              spinning = false;
              color_full(current_color);
              setValueSentOnAllFaces(RANDOMIZE);
              listen_for_player=true;
            }
          }
        }else{
          if(!timer_started && listen_for_player){
            timer_started = true;
            last_time = millis();
            spins_done = 0;
          }else {
            if(listen_for_player){
              if(millis() - last_time >= 1000){       //change 1000 to a variable that will decrease as the rounds continue
                last_time = millis();
                color_face(FAIL_COLOR,spins_done);
                spins_done++;
                if(spins_done >= 6){
                    timer_started = false;
                    listen_for_player = false;
                    setValueSentOnAllFaces(GAME_END);
                }
              }
            }else{
              if(buttonSingleClicked()){
                reset_game();
              }
            }
          }
          
        }
      }
    break;
    case FOLLOWER:
          FOREACH_FACE(f) {
            if(didValueOnFaceChange(f)){
              if(getLastValueReceivedOnFace(f) == RESET){
                setValueSentOnAllFaces(RESET);
                tile_type = UNASSIGNED;
                game_running = false;
                listen_for_player=false;
              }else if(getLastValueReceivedOnFace(f) == RANDOMIZE){
                create_random_colors();
                follower_disabled = false;
                setValueSentOnAllFaces(RANDOMIZE);
                listen_for_player=true;
              }else if(getLastValueReceivedOnFace(f) >= DESIRED_COLOR_START && getLastValueReceivedOnFace(f)<DESIRED_COLOR_END){
                current_color = getLastValueReceivedOnFace(f)-DESIRED_COLOR_START;
                setValueSentOnAllFaces(getLastValueReceivedOnFace(f));
                //color_full(current_color);
              }else if(getLastValueReceivedOnFace(f) == GAME_END){
                listen_for_player = false;
              }else{
                
              }
            }else{
              if(getLastValueReceivedOnFace(f) == IM_PLAYER && listen_for_player){
                if(buttonSingleClicked()){
                  if(face_colors[f] == current_color){
                    setValueSentOnFace(PLAYER_WIN,f);
                    listen_for_player = false;
                    follower_disabled = true;
                    color_full(FAIL_COLOR);
                  }else{
                    setValueSentOnFace(PLAYER_LOSS,f);
                  }
                }else if(!listen_for_player){
                  setValueSentOnFace(PLAYER_OFF,f);
                }
              }
            }
        }
        
    break;
    default:
    break;
  }
}
