#include "lib.h"
#include <stdio.h>
#include <cmath>
#include <fstream>

double beta(double x, double y);
double radius(double x, double y);
double x_rad(double b, double r);
double y_rad(double b, double r);
void draw_arc(double beta_s, double beta_e, double r, int net, double shift);
void draw_arc(double beta_s, double beta_e, double r, int net);
void draw_line(double xs, double ys, double xe, double ye, int net, std::string layer);
void draw_via(double x, double y, int net);
void draw_via_g(double x, double y);                   // draw ground via at x, y
void draw_line_05(double xs, double ys, double xe, double ye, int net, std::string layer);
void draw_arc_05(double beta_s, double beta_e, double r, int net);
void draw_circularzone(double r);
void draw_halfring(double r, double edge);

std::ofstream f;
std::string line1;

double pi = 3.14159265359;

int main() {

    std::string file_name = "/Users/ludwin/Elektronik/rotating_sphere/rotating_sphere_electronics/Sek_Coil_sphere/helper/output.txt";
    f.open(file_name);

    double s_x = 0;
    double s_y = -30.7;
    double vertical_shift = -0.45;
    s_y = s_y - vertical_shift;
    double s_b = beta(0, -30.7);
    double s_r = radius(0, s_y);

    double e_x = 24.85;
    double e_y = 16.38;
    e_y = e_y - vertical_shift;
    double e_b = beta(e_x, e_y);

    draw_arc(s_b, e_b, s_r, 11, -0.45);

    draw_circularzone(29);

    draw_halfring(46.3, -60);

    draw_halfring(46.3, 60);

    double h_x = -32;
    double h_y = -36;
    double h_b = beta(h_x, h_y);
    double h_r = radius(h_x, h_y);
    double he_x = 0;
    double he_y=-50;
    double he_b = beta(he_x, he_y);
    draw_arc(h_b, he_b, h_r, 14);

    f.close();

    return 0;
}

double beta(double x, double y) {
    double beta = atan(x/y);
    if ((x<0)&&(y>0)) beta = pi+beta;
    if ((x>0)&&(y>0)) beta = beta-pi;
    return beta;
}

double radius(double x, double y) {
    double radi = sqrt(x*x+y*y);
    return radi;
}

double x_rad(double b, double r) {
    double x_r = -r * sin(b);
    return x_r;
}

double y_rad(double b, double r) {
    double y_r = -r * cos(b);
    return y_r;
}

void draw_arc(double beta_s, double beta_e, double r, int net, double shift) {
    double delta_arc = 6/(2*pi*62);
    int max;
    if (beta_e<beta_s) {
        max = (beta_s-beta_e)/delta_arc;
    } else {
        max = -(beta_s-beta_e)/delta_arc;
        delta_arc=-delta_arc;
    }
    double start_x = x_rad(beta_s, r);
    double start_y = y_rad(beta_s, r)+shift;
    double end_x, end_y;
    printer ("max: "+std::to_string(max)+" beta_s:"+std::to_string(beta_s)+" beta_e: "+std::to_string(beta_e)+" r: "+std::to_string(r)+"\n");

    for (int i=1; i<max; i++) {
        end_x = x_rad(beta_s-i*delta_arc, r);
        end_y = y_rad(beta_s-i*delta_arc, r)+shift;
        draw_line (start_x, start_y, end_x, end_y, net, "B.Cu");
        start_x = end_x;
        start_y = end_y;
    }
    draw_line (start_x, start_y, x_rad(beta_e, r), y_rad(beta_e, r), net, "B.Cu");
}

void draw_arc(double beta_s, double beta_e, double r, int net) {
    double delta_arc = 6/(2*pi*62);
    int max;
    if (beta_e<beta_s) {
        max = (beta_s-beta_e)/delta_arc;
    } else {
        max = -(beta_s-beta_e)/delta_arc;
        delta_arc=-delta_arc;
    }
    double start_x = x_rad(beta_s, r);
    double start_y = y_rad(beta_s, r);
    double end_x, end_y;
    printer ("max: "+std::to_string(max)+" beta_s:"+std::to_string(beta_s)+" beta_e: "+std::to_string(beta_e)+" r: "+std::to_string(r)+"\n");

    for (int i=1; i<max; i++) {
        end_x = x_rad(beta_s-i*delta_arc, r);
        end_y = y_rad(beta_s-i*delta_arc, r);
        draw_line (start_x, start_y, end_x, end_y, net, "F.Cu");
        start_x = end_x;
        start_y = end_y;
    }
    draw_line (start_x, start_y, x_rad(beta_e, r), y_rad(beta_e, r), net, "F.Cu");
}

void draw_arc_05(double beta_s, double beta_e, double r, int net) {
    double delta_arc = 6/(2*pi*62);
    int max;
    if (beta_e<beta_s) {
        max = (beta_s-beta_e)/delta_arc;
    } else {
        max = -(beta_s-beta_e)/delta_arc;
        delta_arc=-delta_arc;
    }
    double start_x = x_rad(beta_s, r);
    double start_y = y_rad(beta_s, r);
    double end_x, end_y;
    printer ("max: "+std::to_string(max)+" beta_s:"+std::to_string(beta_s)+" beta_e: "+std::to_string(beta_e)+" r: "+std::to_string(r)+"\n");

    for (int i=1; i<max; i++) {
        end_x = x_rad(beta_s-i*delta_arc, r);
        end_y = y_rad(beta_s-i*delta_arc, r);
        draw_line_05 (start_x, start_y, end_x, end_y, net, "F.Cu");
        start_x = end_x;
        start_y = end_y;
    }
    draw_line_05 (start_x, start_y, x_rad(beta_e, r), y_rad(beta_e, r), net, "F.Cu");
}

void draw_line(double xs, double ys, double xe, double ye, int net, std::string layer) {
    line1 ="(segment (start "+std::to_string(xs)+" "+std::to_string(ys)+") (end "+std::to_string(xe)+" "+std::to_string(ye)+") (width 0.25) (layer """+layer+""") (net "+std::to_string(net)+") (uuid ""17aabb6f-34a1-466f-abac-eb0441075269""))\n";
//    printer(line1);
    f<<line1;
}

void draw_line_05(double xs, double ys, double xe, double ye, int net, std::string layer) {
    line1 ="(segment (start "+std::to_string(xs)+" "+std::to_string(ys)+") (end "+std::to_string(xe)+" "+std::to_string(ye)+") (width 0.5) (layer """+layer+""") (net "+std::to_string(net)+") (uuid ""17aabb6f-34a1-466f-abac-eb0441075269""))\n";
//    printer(line1);
    f<<line1;
}


void draw_via(double x, double y, int net) {
    line1 ="(via (at "+std::to_string(x)+" "+std::to_string(y)+") (size 0.5) (drill 0.3) (layers ""F.Cu"" ""B.Cu"") (free yes) (net "+std::to_string(net)+") (uuid ""17aabb6f-34a1-466f-abac-eb0441075269""))\n";
//    printer(line1);
    f<<line1;
}

void draw_via_g(double x, double y) {
    line1 ="(via (at "+std::to_string(x)+" "+std::to_string(y)+") (size 0.5) (drill 0.3) (layers ""F.Cu"" ""B.Cu"") (free yes) (net 1) (uuid ""17aabb6f-34a1-466f-abac-eb0441075269""))\n";
//    printer(line1);
    f<<line1;
}

void draw_circularzone(double r) {
    line1 = "(polygon (pts ";
    for (int i=0; i<50; i++) {
        double alpha = 2*pi/50*i;
        double x = r * sin(alpha);
        double y = r * cos(alpha);
        line1 = line1 + "(xy " + std::to_string(x) + " " + std::to_string(y) +") ";
    }
    line1 = line1 + " ) )\n";
    f<<line1;
}

void draw_halfring(double r, double edge) { // edge is -value for left side, + value for right side
    line1 = "(polygon (pts (xy 0 "+ std::to_string(-r) + ") (xy 0 -60) (xy "+std::to_string(edge)+" -60) (xy "+std::to_string(edge)+" 60) (xy 0 60) (xy 0 "+ std::to_string(r) + ") ";
    for (int i=0; i<25; i++) {
        double alpha = pi/25*i;
        double x = r * - sin(alpha);
        if (edge>0) x=-x;
        double y = r * cos(alpha);
        line1 = line1 + "(xy " + std::to_string(x) + " " + std::to_string(y) +") ";
    }
    line1 = line1 + " ) )\n\n";
    f<<line1;
}
