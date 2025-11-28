#ifndef SDCARD_H
#define SDCARD_H

#include "ff.h" // FatFs library header

void cd(int argc, char *argv[]);
int input(char *filename, char *weatherdata);
void ls(int argc, char *argv[]);
void mkdir(int argc, char *argv[]);
void mount();
void pwd(int argc, char *argv[]);
void rm(char *filename);
int cat(char *filename, char *a, char *b, char *c);
void append(int argc, char *argv[]);
void date(int argc, char *argv[]);
void restart(int argc, char *argv[]);

#endif