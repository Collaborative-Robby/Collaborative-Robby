#ifndef ROBBY_FRACTION_H
#define ROBBY_FRACTION_H

class Fraction {
public:
	unsigned long int n, d;
	Fraction();
	Fraction(unsigned long int n, unsigned long int d);
	Fraction(Fraction *f1, Fraction *f2);
	void normalize();

	friend bool operator== (Fraction &f1, Fraction &f2);
	friend bool operator!= (Fraction &f1, Fraction &f2);
	friend bool operator>  (Fraction &f1, Fraction &f2);
	friend bool operator>= (Fraction &f1, Fraction &f2);
	friend bool operator<  (Fraction &f1, Fraction &f2);
	friend bool operator<= (Fraction &f1, Fraction &f2);
};

#endif /* ROBBY_FRACTION_H */
