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
Fl_Image* jpeg_image;
#define MAX_SIZE_X 10
#define MAX_SIZE_Y 10
#define SCREEN_SIZE_X 1024
#define SCREEN_SIZE_Y 768
int robby_size=70;
int SIZE_X=3;
int SIZE_Y=3;

bool RUN=true;
float speed=1.0;
Fl_Window *window = new Fl_Window(SCREEN_SIZE_X,SCREEN_SIZE_Y,"Robby UI");
Fl_Box* b[MAX_SIZE_X][MAX_SIZE_Y];	
Fl_Button *stop;
Fl_Button *start;
Fl_Button *step;

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
class Scena{
	
	public :
		int map[MAX_SIZE_X][MAX_SIZE_Y];
		void setLine(int x,string l){
			for(int i=0;i<SIZE_Y;i++){
				string tock=l.substr(3*i+i,3);
				//cout << "TOCK |" << tock <<"|\n";
				if(tock==" c "){
					//cout << "lattina in ["<<x<<", "<< i << "]\n";
					map[x][i]=-1;
				}
				if(tock[0]=='R'){
					//cout << "robot in ["<<x<<", "<< i << "]\n";
					map[x][i]=1;
				}
				if(tock=="   "){
					//cout << "vuoto in ["<<x<<", "<< i << "]\n";
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
					//cout<< "FINE DELLA RUN"<<"\n";
				}else
				if(line.substr(0,3)=="-->"){
					//cout << "TROVATA NUOVA SCENA DA CALCOLARE"<<"\n";
					if(prima){
						//cout<< "E' LA PRIMA SCENA"<<"\n";
					}else{
						//cout<<"NON SIAMO ALLA PRIMA"<<"\n";
						first.push_back(*bappo);
						bappo=new Scena();
					}
					prima=false;
					deep=0;
				}else{
					//"[  R0   c      ]"
					string l=line.substr(3,line.size()-4);
					SIZE_Y=(l.size()-2)/3;
					//cout << "dimensione = "<<SIZE_Y << '\n';
					(*bappo).setLine(deep,l);
					//cout << "FATTO \n";
					//(*bappo).stampa();
					deep=deep+1;
				}
			}
		}
		myfile.close();
		/* STAMPA DI DEBUG
		  for (list<Scena>::iterator it=first.begin(); it != first.end(); ++it){
			(*it).stampa();
		}*/		
	}
	else cout << "Unable to open file"; 
	
}
void drawScene(Fl_Window* window,Fl_Box* b[][MAX_SIZE_Y],int scene[][MAX_SIZE_Y]){
	
  Fl_PNG_Image  *robby_orig = new Fl_PNG_Image("img/robby.png");
  Fl_PNG_Image  *trash_orig = new Fl_PNG_Image("img/trash.png");
  for(int i=0;i<SIZE_X;i++){
	  for(int j=0;j<SIZE_Y;j++){
		   //b[i][j] = new Fl_Box(5+i*robby_size,15+j*robby_size,robby_size,robby_size,"");
		   
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

void Open_CB(Fl_Widget *w, void *) {
	 Fl_File_Chooser chooser("run/",                        // directory
                            "*",                        // filter
                            Fl_File_Chooser::SINGLE,     // chooser type
                            "Chose the run to be shown");        // title
    chooser.show();

    // Block until user picks something.
    //     (The other way to do this is to use a callback())
    //
    while(chooser.shown())
        { Fl::wait(); }

    // User hit cancel?
    if ( chooser.value() == NULL )
        { fprintf(stderr, "(User hit 'Cancel')\n"); return; }

    // Print what the user picked
    fprintf(stderr, "--------------------\n");
    fprintf(stderr, "DIRECTORY: '%s'\n", chooser.directory());
    fprintf(stderr, "    VALUE: '%s'\n", chooser.value());
    fprintf(stderr, "    COUNT: %d files selected\n", chooser.count());

	loadScene(chooser.value());
	fl_alert("Selected: %s \n", chooser.value());
	start->activate();
	step->activate();
	//run();
    // Multiple files? Show all of them
    /*if ( chooser.count() > 1 ) {
        for ( int t=1; t<=chooser.count(); t++ ) {
            fprintf(stderr, " VALUE[%d]: '%s'\n", t, chooser.value(t));
        }
    }*/
	
}
void Stop_CB(Fl_Widget *w, void *) {
	RUN=false;
	stop->deactivate();
}
void Quit_CB(Fl_Widget *, void *) {
    exit(0);
}
void Start_CB(Fl_Widget *, void *) {
    RUN=true;
    run();
    stop->activate();
}
void Step_CB(Fl_Widget * q, void *) {
	RUN=false;
    callback(q);
}


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
	//string nome="run/run.txt";
	//loadScene(nome.c_str());
	
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
