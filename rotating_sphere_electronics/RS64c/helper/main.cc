#include "lib.h"
#include <stdio.h>
#include <cmath>
#include <fstream>

double beta(double x, double y);
double radius(double x, double y);
double x_rad(double b, double r);
double y_rad(double b, double r);
void draw_arc(double beta_s, double beta_e, double r, int net);
void draw_line(double xs, double ys, double xe, double ye, int net, std::string layer);
void draw_via(double x, double y, int net);
void draw_via_g(double x, double y);                   // draw ground via at x, y

double LED_x(int LED_nr, int pin);
double LED_y(int LED_nr, int pin);

double rule_here(double b);
void generate_edges();
void add_edge(double b);
void sort_edges();
void occupy_rad();
void fix_mini();
int compare_desc(const void *a, const void *b);
void print_edges();

void add_obst(double beta_start, double beta_end, double r);
void add_obst_via(double vx, double vy);
void add_obst_arc(double abs, double abe, double ar);

void draw_line_05(double xs, double ys, double xe, double ye, int net, std::string layer);
void draw_arc_05(double beta_s, double beta_e, double r, int net);

std::ofstream f;
std::string line1;

double rules_beta_start[60000];
double rules_beta_end[60000];
double rules_rad[60000];
int rules_nr=0;
double edge_seq[10000];
int edge_nr=0;
double rad_seq[10000];

int main() {
    printer("LED data\n");

    std::string file_name = "/Users/ludwin/Elektronik/rotating_sphere/rotating_sphere_electronics/RS64c/helper/output.txt";
    f.open(file_name);

// =======================================================
// 8er Anodenleitungen

    double b_start = beta(-76.3, 55);
    int lookup1[] = {19, 42, 43, 44, 48, 47, 46, 45};

    for (int gruppe=0; gruppe<4; gruppe++) {
        double schraeg = 0.4;
        double an_r = 79.5 + (double)gruppe * 0.41;
        double b_end = beta(LED_x((gruppe*8+7), 2), LED_y((gruppe*8+7), 2))+ schraeg/an_r;
        draw_arc(b_start, b_end, an_r, lookup1[gruppe]);
        for (int g_led=0; g_led<8; g_led++) {
            int LED = g_led+gruppe*8;
            double  t_b = beta(LED_x(LED,2), LED_y(LED,2));
            double t_r = an_r;
            double z_b = t_b+schraeg/t_r;
            draw_line( LED_x(LED, 2), LED_y(LED, 2), x_rad(t_b, t_r-schraeg),y_rad(t_b, t_r-schraeg), lookup1[gruppe], "F.Cu");
            draw_line( x_rad(t_b, t_r-0.4),y_rad(t_b, t_r-0.4), x_rad(z_b, t_r),y_rad(z_b, t_r), lookup1[gruppe], "F.Cu");
        }
    }

    for (int gruppe=4; gruppe<8; gruppe++) {
        double schraeg = 0.4;
        double an_r = 79.5 + (7.0-(double)gruppe) * 0.41;
        double b_end = beta(LED_x((gruppe*8), 2), LED_y((gruppe*8), 2))- schraeg/an_r;
        draw_arc(-b_start, b_end, an_r, lookup1[gruppe]);
        for (int g_led=0; g_led<8; g_led++) {
            int LED = g_led+gruppe*8;
            double  t_b = beta(LED_x(LED,2), LED_y(LED,2));
            double t_r = an_r;
            double z_b = t_b-schraeg/t_r;
            draw_line( LED_x(LED, 2), LED_y(LED, 2), x_rad(t_b, t_r-schraeg),y_rad(t_b, t_r-schraeg), lookup1[gruppe], "F.Cu");
            draw_line( x_rad(t_b, t_r-0.4),y_rad(t_b, t_r-0.4), x_rad(z_b, t_r),y_rad(z_b, t_r), lookup1[gruppe], "F.Cu");
        }
    }

// 24 Kathodenringe
//
    int lookup2[]={17, 18, 20,  23, 22, 21,  24, 26, 25,  27, 28, 29,    32, 30, 31,  34, 35, 33,  37, 38, 36,  41, 40, 39};
    double kr0_r = 76.2;
    int pins_left[]={3, 4, 1};
    int pins_right[]={1, 4, 3};

    rules_beta_start[0]=b_start+20/kr0_r;
    rules_beta_end[0]=-rules_beta_start[0];
    rules_rad[0]=kr0_r;
    rules_nr=1;

    generate_edges();
    print_edges();

    for (int kr=0; kr<24; kr++) {

        if (kr==0) {
            draw_arc(b_start, -b_start, kr0_r, lookup2[kr]);  // obstacle list ist bereits initialisiert
        }
        else {

            // ==============================
            // zeichne Bogen mit Hindernis-Unterbrechnungen
            for (int index = 0; index<edge_nr-1; index++) {
                double bs = edge_seq[index];
                double be = edge_seq[index+1];
                double rs = rad_seq[index]-0.43;
                double re = rad_seq[index+1]-0.43;
                if ((rs!=re)||(bs!=be)) {
                    draw_arc(bs, be, rs, lookup2[kr]);
                    draw_line(x_rad(be, rs), y_rad(be, rs), x_rad(be, re), y_rad(be, re), lookup2[kr], "F.Cu");
                    add_obst(bs, be, rs);
                }
                if (re!=rs) {
                    double kl = re;
                    if (rs<re) kl=rs;
                    add_obst(be+0.41/rs, be-0.41/rs, kl);
                }
            }

            generate_edges();
            print_edges();
        }

        for (int gruppe=0; gruppe<8; gruppe++) {                    // <8 !!
            int led_index = kr/3;
            int pin_index = kr%3;
            if (gruppe>3) led_index = 7-led_index;
            int LED_N = gruppe*8+led_index;
            printer ("gruppe: "+std::to_string(gruppe)+"   led_index: "+std::to_string(led_index)+"   LED_N: "+std::to_string(LED_N)+"\n");
            int pin_N;
            if (kr<12) pin_N=pins_left[pin_index];
            else pin_N=pins_right[pin_index];

            double pinx = LED_x(LED_N, pin_N);
            double piny = LED_y(LED_N, pin_N);
            double pin_beta = beta(pinx, piny);

            draw_via_g(x_rad(pin_beta+0.62/84, 78.-1.21), y_rad(pin_beta+0.62/84, 78.-1.21));

            if (kr==0) {
                draw_line(pinx, piny, x_rad(pin_beta, kr0_r-0.41*kr), y_rad(pin_beta, kr0_r-0.41*kr), lookup2[kr], "F.Cu");
            }
            if (kr>0) {
                double r_via = rule_here(pin_beta)-0.15;
                draw_line(pinx, piny, x_rad(pin_beta, r_via), y_rad(pin_beta, r_via), lookup2[kr], "B.Cu");
                draw_via(x_rad(pin_beta, r_via), y_rad(pin_beta, r_via), lookup2[kr]);
                add_obst_via(x_rad(pin_beta, r_via), y_rad(pin_beta, r_via));
                double vg_b = pin_beta+0.65/r_via;
                double vg_r = rule_here(vg_b)-0.16;
                double dx=x_rad(pin_beta, r_via)-x_rad(vg_b, vg_r);
                double dy=y_rad(pin_beta, r_via)-y_rad(vg_b, vg_r);
                double dist = sqrt(dx*dx+dy*dy);
                if ((dist)>0.7) {
                    vg_r=r_via-0.3;
                    vg_b=pin_beta;
                }
                draw_via_g(x_rad(vg_b, vg_r-0.42), y_rad(vg_b, vg_r-0.42));
                add_obst_via(x_rad(vg_b, vg_r-0.42), y_rad(vg_b, vg_r-0.42));
            }
        }
        generate_edges();
        print_edges();


    }


    f.close();

    return 0;
}


// =======================================================

double beta(double x, double y) {
    double pi = 3.14159265359;
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

void draw_arc(double beta_s, double beta_e, double r, int net) {
    double pi = 3.14159265359;
    double delta_arc = 10/(2*pi*62);
    int max;
    if (beta_e<beta_s) {
        max = (int)((beta_s-beta_e)/delta_arc);
    } else {
        max = (int)(-(beta_s-beta_e)/delta_arc);
        delta_arc=-delta_arc;
    }
    double start_x = x_rad(beta_s, r);
    double start_y = y_rad(beta_s, r);
    double end_x, end_y;

    for (int i=1; i<max; i++) {
        end_x = x_rad(beta_s-i*delta_arc, r);
        end_y = y_rad(beta_s-i*delta_arc, r);
        if ((start_x!=end_x)||(start_y!=end_y)) draw_line (start_x, start_y, end_x, end_y, net, "F.Cu");
        start_x = end_x;
        start_y = end_y;
    }
    if ((start_x != x_rad(beta_e, r))||(start_y!=y_rad(beta_e, r)))draw_line (start_x, start_y, x_rad(beta_e, r), y_rad(beta_e, r), net, "F.Cu");
}

void draw_arc_05(double beta_s, double beta_e, double r, int net) {
    double pi = 3.14159265359;
    double delta_arc = 6/(2*pi*62);
    int max;
    if (beta_e<beta_s) {
        max = (int)((beta_s-beta_e)/delta_arc);
    } else {
        max = (int)(-(beta_s-beta_e)/delta_arc);
        delta_arc=-delta_arc;
    }
    double start_x = x_rad(beta_s, r);
    double start_y = y_rad(beta_s, r);
    double end_x, end_y;

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
    line1 ="(segment (start "+std::to_string(xs)+" "+std::to_string(ys)+") (end "+std::to_string(xe)+" "+std::to_string(ye)+") (width 0.2) (layer """+layer+""") (net "+std::to_string(net)+") (uuid ""17aabb6f-34a1-466f-abac-eb0441075269""))\n";
    if ((xs!=xe)||(ys!=ye)) f<<line1;
}

void draw_line_05(double xs, double ys, double xe, double ye, int net, std::string layer) {
    line1 ="(segment (start "+std::to_string(xs)+" "+std::to_string(ys)+") (end "+std::to_string(xe)+" "+std::to_string(ye)+") (width 0.5) (layer """+layer+""") (net "+std::to_string(net)+") (uuid ""17aabb6f-34a1-466f-abac-eb0441075269""))\n";
    f<<line1;
}


void draw_via(double x, double y, int net) {
    line1 ="(via (at "+std::to_string(x)+" "+std::to_string(y)+") (size 0.5) (drill 0.3) (layers ""F.Cu"" ""B.Cu"") (free yes) (net "+std::to_string(net)+") (uuid ""17aabb6f-34a1-466f-abac-eb0441075269""))\n";
    f<<line1;
}

void draw_via_g(double x, double y) {
    line1 ="(via (at "+std::to_string(x)+" "+std::to_string(y)+") (size 0.5) (drill 0.3) (layers ""F.Cu"" ""B.Cu"") (free yes) (net 2) (uuid ""17aabb6f-34a1-466f-abac-eb0441075269""))\n";
    f<<line1;
}


double LED_x(int LED_nr, int pin) {
    if (LED_nr>31) pin = 5-pin;
    double width = 5.5;
    double rLED = 84.0;
    double be = (width/4 + width * (31-LED_nr))/rLED;
    double rk = 78.0;
    double y = -((double)pin - 2.5)*1.27;
    double rs = sqrt(y*y+rk*rk);
    double delta = atan(y/rk);
    double bs = be-delta;
    return x_rad(bs, rs);
}

double LED_y(int LED_nr, int pin) {
    if (LED_nr>31) pin = 5-pin;
    double width = 5.5;
    double rLED = 84.0;
    double be = (width/4 + width * (31-LED_nr))/rLED;
    double rk = 78.0;
    double y = -((double)pin - 2.5)*1.27;
    double rs = sqrt(y*y+rk*rk);
    double delta = atan(y/rk);
    double bs = be-delta;
    return y_rad(bs, rs);
}



double rule_here(double b) {          // gibt den Radius zurück, der bei b gilt
    double here = 80.;
    for (int i=0; i<rules_nr; i++) {
        if ((rules_beta_start[i]>=b)&&(rules_beta_end[i]<=b)) {
            if (rules_rad[i]<here) here=rules_rad[i];
        }
    }
    return here;
}

void add_obst(double beta_start, double beta_end, double r) {
//    printer ("add_obst: beta_start"+std::to_string(beta_start)+"  beta_end: "+std::to_string(beta_end) + "  r: " + std::to_string(r)+"  rules_nr: "+std::to_string(rules_nr)+"\n");
    rules_beta_start[rules_nr]=beta_start;
    rules_beta_end[rules_nr]=beta_end;
    rules_rad[rules_nr]=r;
    rules_nr++;
}

void generate_edges() {
    edge_nr=0;
    for (int i = 0; i<rules_nr; i++) {
        add_edge(rules_beta_start[i]);
        add_edge(rules_beta_end[i]);
    }
    sort_edges();
    occupy_rad();
    fix_mini();
}

void add_edge(double b) {
    bool does_exist=false;
    for (int k=0; k<edge_nr; k++) {
        if (edge_seq[k]==b) does_exist=true;
    }
    if (does_exist==false) {
        edge_seq[edge_nr]=b;
        edge_nr++;
    }
}

void sort_edges() {
    qsort(edge_seq, (size_t)edge_nr, sizeof(double), compare_desc);
}

void occupy_rad() {
    for (int i = 0; i<edge_nr; i++) {
        double mid;
        if (i!=edge_nr-1) {
            mid = (edge_seq[i]+edge_seq[i+1])/2;
        } else {
            mid = edge_seq[i];
        }
        rad_seq[i]=rule_here(mid);
    }
}

void fix_mini() {
    for (int i = 1; i<edge_nr-20; i++) {
        if (rad_seq[i]>rad_seq[i-1]) {
            int k=0;
            for (k=1; k<20;k++) {
                if (rad_seq[i+k]<rad_seq[i]) break;
            }
            if (k<20) {
                if (fabs(edge_seq[i]-edge_seq[i+k])<4.0/rad_seq[i]) {
                    for (int m=0; m<k; m++) rad_seq[i+m]=rad_seq[i-1];
                }
            }

        }
    }
}

int compare_desc(const void *a, const void *b) {
    double da = *(const double*)a;
    double db = *(const double*)b;
    return (db > da) - (db < da); // Umgekehrte Reihenfolge
}

void add_obst_via(double vx, double vy) {
    double vb = beta(vx, vy);
    double vr = radius(vx, vy);
    double vb1 = vb+0.57/vr;
    double vb2 = vb-0.57/vr;
    double vr1 = vr-0.135;
    add_obst(vb1, vb2, vr1);
}


void add_obst_arc(double abs, double abe, double ar) {
    add_obst(abs, abe, ar-0.12);
}

void print_edges() {
    for(int m=0; m<edge_nr; m++) {
        printer("aufsteigend >>> edge_nr: "+std::to_string(m)+"   edge_seq: "+std::to_string(edge_seq[m])+"\n");

    }
}
