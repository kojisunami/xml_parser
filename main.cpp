#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <vector>

using namespace std;

#define BUF_SIZE 32768

#define ID_ALLY 1
#define ID_ENEMY 2

const char filename[] = "hoge.xml";


class Xml_Bracket{
public:
	char *name;
	
	char **arg;

	char *buf;
	int arg_len[32];
	int num_arg;

	//char *dat;
	int flag;	 // start or end;
	void str_spacer(char *str, int len);
};

class Xml_Tree{
public:
	int fd ;

	char buf[BUF_SIZE];

	int gen;
	int size;
	vector<Xml_Bracket> xml_list;

	vector<Xml_Tree>children;

	void load(const char filename[]);
	char *xml_type;

	void lex();
	int bracket_start(int off );
	int bracket_end(int off);

	char *get_data(char* target, char *keyword);

	void show();
	void show_buf();

	int type();
	int create_children(void);


	Xml_Tree(const char filename[]){
		gen = 0;
		load(filename);
		lex();
		type();
		create_children();
	}
	Xml_Tree(vector<Xml_Bracket> new_xml_list, int parent_gen){
		xml_list = new_xml_list;
		gen = parent_gen + 1;
		create_children();
	}

		
};

class Unit{
public:
	char *name;
	int id;
	int lv;
	int hp;
	int mp;
	int atk;
	int def;
};

int Xml_Tree::bracket_start(int off){
	for(int i=off;i<size;i++)
		if(buf[i] == '<')
			return i;
	return -1;
}

int Xml_Tree::bracket_end(int off){
	for(int i=off;i<size;i++)
		if(buf[i] == '>')
			return i;
	return -1;
}


int str_match(char *s0, char *s1, int len){
	for(int i =0;i<len;i++){
		if(s0[i] != s1[i])
			return -1;	
	}
	return 0;
}


// this function should only be called for root branch
int Xml_Tree::type(){

	// search for header information <!DOCTYPE>
	if( str_match(xml_list[0].name,(char *)"!DOCTYPE", 8) == 0){
		//printf("header found\n");
		xml_type = (char*) malloc(sizeof(char)*xml_list[0].arg_len[1]);
		xml_type = xml_list[0].arg[1];
		printf("xml type is %s\n", xml_type);
		xml_list.erase(xml_list.begin());
	}
	return 0;
}

void Xml_Bracket::str_spacer(char *str, int len){
	
	num_arg=0;
	for(int i=0;i<32;i++)
		arg_len[i] = 0;
	int flag=0;
	
	// find num of args 
	for(int i =0;i<len;i++){
		if( (str[i] == 0x0a) || (str[i] == 0x00) || (i == len-1)){
			if(flag ==1)
				num_arg++;
			break;
		}
		else if( (str[i] != ' ') && (str[i] != '\t')){
			flag = 1;
		}
		else if(flag == 1){
			flag = 0;
			num_arg++;
		}
	}
	arg = (char**) malloc(sizeof(char)*num_arg);
	for(int i =0;i<num_arg;i++)
		arg[i]=(char*)malloc(sizeof(char)*256);


	// put each 
	num_arg = 0;
	for(int i =0;i<len;i++){
		if( (str[i] == 0x0a) || (str[i] == 0x00) || (i == len-1)){
			if(flag ==1){
				arg[num_arg][arg_len[num_arg]] = '\0';
				num_arg++;
			}
			break;
		}
		else if( (str[i] != ' ') && (str[i] != '\t')){
			flag = 1;
			arg[num_arg][arg_len[num_arg]] = str[i];
			arg_len[num_arg]++;
		}
		else if(flag == 1){
			flag = 0;
			num_arg++;
			arg_len[num_arg] = 0;
		}
	}
	name = arg[0];
}

int Xml_Tree::create_children(){

	vector<vector<Xml_Bracket>> children_xml_list;
	vector<Xml_Bracket> tmp_xml_list;	

	int nest = 0;
	int num_children = 0;
	for(int i =1;i<xml_list.size()-1;i++){
		if(xml_list[i].flag == 1){
			nest ++ ;
		}else{
			nest--;
		}
		tmp_xml_list.push_back(xml_list[i]);
		if(nest == 0){
			num_children++;
			children_xml_list.push_back(tmp_xml_list);
			tmp_xml_list.resize(0);
		}
	}

	//printf("num_childen is %d\n", num_children);

	for(int i =0;i<children_xml_list.size();i++){
		Xml_Tree tmp (children_xml_list[i], gen);
		children.push_back(tmp);
	}

	// if everything is done, there only remain two elements in parent branch
	tmp_xml_list = xml_list;
	xml_list.resize(0);
	xml_list.push_back(tmp_xml_list[0]);
	xml_list.push_back(tmp_xml_list[tmp_xml_list.size()-1]);

	return 0;
}

void Xml_Tree::lex(){
	
	int start,end;

	char *name;

	Xml_Bracket xml_bracket;

	for(int i=0;i<size;i++){
		if( ((start = bracket_start(i)) != -1) &&  ((end = bracket_end(start)) != -1)){

			// retrieve the string inside of the bracket
			name = (char*)malloc(sizeof(char) * (end-start));

			if(buf[start+1] == '/'){
				xml_bracket.flag = 1;
				for(int j =start+2;j<end;j++)
					name[j-start-2] = buf[j];
				xml_bracket.str_spacer(name, end-start-1);
			} else{
				xml_bracket.flag = 0;
				for(int j =start+1;j<end;j++)
					name[j-start-1] = buf[j];
				xml_bracket.str_spacer(name, end-start);
			}

			// analyze the string

		}else{
			return;
		}
		i = end;

		xml_list.push_back(xml_bracket);
	}
}

// warning: this load program is not compatible with more tahan 256 bytes.
void Xml_Tree::load(const char filename[]){

	fd = open( filename, O_RDONLY);

	if(fd == -1){
		write(1, "no such file\n", 13);
		exit(-1);
	}

	size = read(fd, buf, BUF_SIZE);


	
	//for(int i = 0; i< BUF_SIZE;i++);
}

void Xml_Tree::show(){
	int nest = 0;

	for(int i =0;i<xml_list.size();i++){

		// show the name of bracket
		if(xml_list[i].flag == 0){
			for(int j=0;j<nest;j++)
				write(1,"  ",2);
			nest++;
		}else{
			nest--;
			for(int j=0;j<nest;j++)
				write(1,"  ",2);
		}
		printf("%s\n", xml_list[i].name);

		// show the information of bracket
		for(int j = 1; j < xml_list[i].num_arg; j++){
			for(int j=0;j<nest;j++)
				write(1,"  ",2);
			printf("- %s\n",  xml_list[i].arg[j]);
		}
	}
}

void Xml_Tree::show_buf(){
	write(1, buf, size);
}
char *Xml_Tree::get_data(char* target, char *keyword){

	char* str = (char*)malloc(64);

	for(int i =0;i<children.size();i++){
			if( str_match(children[i].xml_list[0].name, target, strlen(target)) == 0){
				int off;
				for(int k=0;k<children[i].xml_list[0].arg_len[1]; k++){
					if( children[i].xml_list[0].arg[1][k] == '\"'){
						off = k+1; break;
					}
				}

				//printf("off is %d\n", off);
				for(int k=0;k<children[i].xml_list[0].arg_len[1] -off; k++){
					if( children[i].xml_list[0].arg[1][k+off] == '\"') {
						str[k]='\0';
						break;
					}
					str[k] = children[i].xml_list[0].arg[1][k+off];
				}
			}
		}
	return str;
}


int main(int argc, char *argv[]){

	Xml_Tree xml_tree(filename);

	//printf("*branch info gen=%d*\n", xml_tree.gen);
	/*
	xml_tree.show();
	printf("\n\n");

	for(int i =0;i<xml_tree.children.size();i++){
		xml_tree.children[i].show();
		printf("\n\n");

		for(int j =0;j<xml_tree.children[i].children.size();j++){
			xml_tree.children[i].children[j].show();
			printf("\n\n");
		}
	}*/

	Unit unit[3];
	printf("xml tree size is %d\n", xml_tree.children.size());
	for(int i =0;i<xml_tree.children.size();i++){
		unit[i].name = xml_tree.children[i].get_data((char*)"name", (char*)"value=");

		if ( str_match(xml_tree.children[i].get_data((char*)"id", (char*)"value="), (char*)"ally", 4) ==0)
			unit[i].id = ID_ALLY;
		else
			unit[i].id = ID_ENEMY;

		unit[i].lv  = atoi(xml_tree.children[i].get_data((char*)"lv", (char*)"value="));
		unit[i].hp  = atoi(xml_tree.children[i].get_data((char*)"hp", (char*)"value="));
		unit[i].mp  = atoi(xml_tree.children[i].get_data((char*)"mp", (char*)"value="));
		unit[i].atk = atoi(xml_tree.children[i].get_data((char*)"atk", (char*)"value="));
		unit[i].def = atoi(xml_tree.children[i].get_data((char*)"def", (char*)"value="));
	}

	for(int i =0;i<xml_tree.children.size();i++){
		printf("\n\n*character info*\n\n");
		printf("name: %s\n", unit[i].name);
		if(unit[i].id == ID_ALLY)
			printf("id  : ally\n");
		if(unit[i].id == ID_ENEMY)
			printf("id  : enemy\n");
		printf("lv  : %d\n", unit[i].lv);
		printf("hp  : %d\n", unit[i].hp);
		printf("mp  : %d\n", unit[i].mp);
		printf("atk : %d\n", unit[i].atk);
		printf("def : %d\n", unit[i].def);
	}

	//printf("len of name is %d\n", strlen("name"));

	return 0;
}






