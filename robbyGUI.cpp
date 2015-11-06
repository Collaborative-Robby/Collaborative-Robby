#include <unistd.h> //funziona su linux mentre per win <windows.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_RGB_Image.H>
#include <FL/Fl_JPEG_Image.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Hor_Slider.H>

#include <iostream>
#include <fstream>
#include <string>
#include <list>


using namespace std;
/**
 * Some constant for the whole computation,
 * MAX_SIZE_X AND MAX_SIZE_Y must be equals
 **/
#define MAX_SIZE_X 10
#define MAX_SIZE_Y 10
/**
 * Some constant for the whole computation,
 * SCREEN_SIZE_X >= SCREEN_SIZE_Y+(75+5)*3+10
 **/
#define SCREEN_SIZE_X 1024
#define SCREEN_SIZE_Y 768
/**
 * robby_size <=(SCREEN_SIZE_Y-50)/MAX_SIZE_Y
 **/
int robby_size=70;
/**
 * SIZE_X <=MAX_SIZE_X
 * SIZE_Y <=MAX_SIZE_Y
 **/
int SIZE_X=3;
int SIZE_Y=3;

bool RUN=true;
float speed=1.0;
Fl_Window *window = new Fl_Window(SCREEN_SIZE_X,SCREEN_SIZE_Y,"Robby UI");
Fl_Box* b[MAX_SIZE_X][MAX_SIZE_Y];	
Fl_Button *stop;
Fl_Button *start;
Fl_Button *step;
Fl_Image* jpeg_image;

int scene[][MAX_SIZE_Y] = {
					  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
					  {0, 1, 0, 0, 0, 0, -1, 0, 0, 0},
					  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
					  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
					  {0, -1, 0, 0, -1, 0, 0, 0, 0, 0},
					  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
					  {0, 0, 0, 0, 0, 0, 0, -1, 0, 0},
					  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
					  {0, 0, 0, 0, 1, -1, 0, 0, 0, 0},
					  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
				  };

/**
 * Each scene represents a specific status of the problem to be solved
 * map is a N*N Matrix of int where 
 * 0 represent void
 * 1 represent robby
 * -1 represent can/garbage 
 **/
class Scena{	
	public :
		int map[MAX_SIZE_X][MAX_SIZE_Y];
		void setLine(int x,string l){
			for(int i=0;i<SIZE_Y;i++){
				string tock=l.substr(3*i+i,3);
				if(tock==" c "){
					map[x][i]=-1;
				}
				if(tock[0]=='R'){
					map[x][i]=1;
				}
				if(tock=="   "){
					map[x][i]=0;
				}
			}
		}
		void stampa(){
			cout << "--------\n";
			for(int i=0;i<SIZE_X;i++){
				cout <<"|";
				for(int j=0;j<SIZE_Y;j++){
					if(map[i][j]==-1){
						cout << 'c';
					}else
					if(map[i][j]==1){
						cout << 'R';
					}else
					if(map[i][j]==0){
						cout << ' ';
					}else{
						cout << '*';
					}
				}
				cout <<"|\n";
			}
			cout <<'\n';
		}
} s;  

list<Scena> first;
/**
 * This method load a list of Scena from file named nome
 * @input nome the name of the file
 * */
void loadScene(const char* nome){
	string line;
	ifstream myfile (nome);
	if (myfile.is_open())
	{
		bool b= false;
		bool prima=true;
		Scena current;
		Scena* bappo=new Scena();
		int deep=0;
		while ( getline (myfile,line) )
		{
			if(line=="===> Test Generation"){
				cout<< "INIZIO DELLA RUN"<<"\n";
				b=true;
			}
			else
			if(b){
				if(line.substr(0,6)=="failed"){
				}else
				if(line.substr(0,8)=="===> End"){
				}else
				if(line.substr(0,3)=="-->"){
					if(prima){
					}else{
						first.push_back(*bappo);
						bappo=new Scena();
					}
					prima=false;
					deep=0;
				}else{
					string l=line.substr(3,line.size()-4);
					SIZE_Y=(l.size()-2)/3;
					(*bappo).setLine(deep,l);
					deep=deep+1;
				}
			}
		}
		myfile.close();
	}
	else cout << "Unable to open file"; 
	
}
/**
 * This method draw a Scene in the window and repaint it
 * @input window the window that will be repainted
 * @input b the matrix of box that contain the robby and the cans
 * @input scene the scene that will be displayed
 **/
void drawScene(Fl_Window* window,Fl_Box* b[][MAX_SIZE_Y],int scene[][MAX_SIZE_Y]){
	
  Fl_PNG_Image  *robby_orig = new Fl_PNG_Image("img/robby.png");
  Fl_PNG_Image  *trash_orig = new Fl_PNG_Image("img/trash.png");
  for(int i=0;i<SIZE_X;i++){
	  for(int j=0;j<SIZE_Y;j++){		   
		   if(scene[i][j]==1){
			   jpeg_image = robby_orig->copy( b[i][j]->w(), b[i][j]->h() );
		   }
		   if(scene[i][j]==-1){
			   jpeg_image = trash_orig->copy( b[i][j]->w(), b[i][j]->h() );
		   }
		   if(scene[i][j]!=0){
			   b[i][j]->image( jpeg_image );
			   b[i][j]->color(FL_WHITE);
		   }else{
			   b[i][j]->image(0);
		   }
	  }
  }
  window->redraw();
}
/**
 * This method will be called after each "speed" by default 1 second.
 * He load a new scene and print it to the screen
 **/
void callback(void*) {
	for(int i=0;i<SIZE_X;i++){
		for(int j=0;j<SIZE_Y;j++){
			scene[i][j]=first.front().map[i][j];
		}
	}
	if(first.size()>0){
		first.pop_front();
		drawScene(window,b,scene);
		puts("TICK");
		if(RUN)
			Fl::repeat_timeout(speed, callback);
	}else{
		fl_alert("Run Completed \n");
		stop->deactivate();
		start->deactivate();
		step->deactivate();
	}
}
/**
 * This method start calling the callback method and start running the show 
 **/
void run(){
	for(int i=0;i<SIZE_X;i++){
		for(int j=0;j<SIZE_Y;j++){
			scene[i][j]=first.front().map[i][j];
		}
	}
	//**scene=**first.front().map;
	first.pop_front();
	drawScene(window,b,scene);
	Fl::add_timeout(1.0,callback);  
}

/**
 * This method let user select the run to be displayed
 **/
void Open_CB(Fl_Widget *w, void *) {
	 Fl_File_Chooser chooser("run/",                        // directory
                            "*",                        // filter
                            Fl_File_Chooser::SINGLE,     // chooser type
                            "Chose the run to be shown");        // title
    chooser.show();
    while(chooser.shown())
        { Fl::wait(); }

    if ( chooser.value() == NULL )
        { fprintf(stderr, "(User hit 'Cancel')\n"); return; }

    fprintf(stderr, "--------------------\n");
    fprintf(stderr, "DIRECTORY: '%s'\n", chooser.directory());
    fprintf(stderr, "    VALUE: '%s'\n", chooser.value());
    fprintf(stderr, "    COUNT: %d files selected\n", chooser.count());

	loadScene(chooser.value());
	fl_alert("Selected: %s \n", chooser.value());
	start->activate();
	step->activate();
	
}
/**
 * This method stop the run
 **/
void Stop_CB(Fl_Widget *w, void *) {
	RUN=false;
	stop->deactivate();
}
/**
 * This method close the program
 **/
void Quit_CB(Fl_Widget *, void *) {
    exit(0);
}
/**
 * This method start the run
 **/
void Start_CB(Fl_Widget *, void *) {
    RUN=true;
    run();
    stop->activate();
}
/**
 * This method move forward of a single step
 **/
void Step_CB(Fl_Widget * q, void *) {
	RUN=false;
    callback(q);
}
/**
 * This method change the step "speed" of the run
 **/
static void Slider_CB(Fl_Widget *w, void *data) {
	Fl_Slider* slider = (Fl_Slider*)w;
	cout << slider->value() << "\n";
	speed=(11+slider->value())/5;
//	slider->value();
}

int main(int argc, char **argv) {
	Fl_Menu_Bar *menu = new Fl_Menu_Bar(0,0,SCREEN_SIZE_X,25);
    menu->add("File/Open",   FL_CTRL+'n', Open_CB);
    menu->add("File/Quit",   FL_CTRL+'q', Quit_CB);
    menu->add("Edit/Start", FL_CTRL+'s', Start_CB);
    menu->add("Edit/Stop", FL_CTRL+'p', Stop_CB);
    
    int btn_width=75;
	stop=new Fl_Button(SCREEN_SIZE_Y+5,100,btn_width,25,"Stop");
	start=new Fl_Button(SCREEN_SIZE_Y+5+btn_width+5,100,btn_width,25,"Start");
	step=new Fl_Button(SCREEN_SIZE_Y+5+(btn_width+5)*2,100,btn_width,25,"Step");
	stop->callback(Stop_CB);
	start->callback(Start_CB);
	step->callback(Step_CB);
	
	stop->deactivate();
	start->deactivate();
	step->deactivate();
	
	Fl_Slider *slider=new Fl_Hor_Slider(SCREEN_SIZE_Y+5,130,(btn_width+5)*3,25,"Speed");
	slider->callback(Slider_CB);
	slider->range(-10,10);
	slider->value(1);
	slider->step(1);
	
	for(int i=0;i<SIZE_X;i++){
		for(int j=0;j<SIZE_Y;j++){
		   b[i][j] = new Fl_Box(5+j*robby_size,25+15+i*robby_size,robby_size,robby_size,"");
		}
	}
	

	window->end();
	window->show(argc, argv);
  return Fl::run();;
}

// per compilare g++ -o go robbyGUI.cpp -L/home/sterling/Downloads/fltk-1.3.x-r8695/FL -lfltk -lXext -lX11 -lm -lfltk_images
