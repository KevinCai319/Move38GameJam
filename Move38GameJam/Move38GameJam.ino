#define GREEN makeColorRGB(0,255,0)
#define FAIL_COLOR makeColorRGB(0,0,0)
//Success color would just be team color to make things easier.

//List of messages that can be sent.
#define SPINNER_SET_MSG 8
#define RESET 9
#define RANDOMIZE 10
#define RESET_ROUND 11

//List of colors that can appear in-game.
const Color COLORS[] = {RED,GREEN,BLUE,YELLOW,MAGENTA,WHITE};
// How many colors to use. Can be less than the COLORS[] array.
const byte NUM_COLORS = 6;

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
bool player_finalized = false;
bool listen_for_player = false;
void setup() {
  //set seed for RNG.
  randomize();
  //Every single tile changes to a random color.
  create_random_colors();
}
//This area meant for followers and uninitialized tiles.
void create_random_colors(){
  FOREACH_FACE(f)setColorOnFace(COLORS[random(NUM_COLORS-1)],f);
}

//Does neighbor at <side> have the right color.
//Arguably the most important method.
//TODO: implement method.
bool same_color_edge(int side){
  
}

//This area meant for spinners.


//TODO: finish this method.
void initialize_game(){
  tile_type = SPINNER;
  setValueSentOnAllFaces(SPINNER_SET_MSG);
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
    }
}

//Set the player's 4 panels that represent team color.
void set_player_team_color(){
  for(int i = 2;i < 6;i++){
    setColorOnFace(COLORS[current_color],i);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  switch(tile_type){
    case UNASSIGNED:
      if(isAlone()){
        //Some user feedback that this device is not connected to anything.
        setColor(WHITE);
        if(buttonDoubleClicked()){
          tile_type = PLAYER;
          setColorOnFace(FAIL_COLOR,0);
          setColorOnFace(FAIL_COLOR,1);
          set_player_team_color();
        }
      }else{
        if(millis()-last_time >= 250){
          create_random_colors();
          last_time = millis();
        }
        if(buttonSingleClicked()){
          //TODO: implement method.
          initialize_game();
          break;
        }
        FOREACH_FACE(f) {
          if(didValueOnFaceChange(f) && getLastValueReceivedOnFace(f) == SPINNER_SET_MSG){
            setColor(WHITE);
            setValueSentOnAllFaces(SPINNER_SET_MSG);
            tile_type = FOLLOWER;
            game_running = true;
          }
        }
        
 
      }
    break;
    case PLAYER:
      if(!player_finalized){
        player_setup();
      }else{
        //TODO: Have the player read in data from sides if any follower tells it to do something.
      }
    break;
    case SPINNER:
      //if there is a triple click, just reset everything so everything is OK.
      if(buttonMultiClicked()){
         tile_type = UNASSIGNED;
         setValueSentOnAllFaces(RESET);
         game_running = false;
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
              setValueSentOnAllFaces(RANDOMIZE);
              listen_for_player=true;
            }else{
              
            }
          }
        }
    break;
    default:
    break;
  }
}
