#include <iostream>
using namespace std;

class Fraction {
public:
	unsigned long int n, d;
	Fraction(unsigned long int n, unsigned long int d)
	{
		this->n = n;
		this->d = d;

		this->normalize();
	}

	/* Middle point constructor */
	Fraction(Fraction *f1, Fraction *f2) {
		if (f1->d >= f2->d) {
			this->d = f1->d * 2;
			/* lcm */
			this->n = (f2->n * (f1->d / f2->d)) + f1->n;
		} else {
			this->d = f2->d * 2;
			/* lcm */
			this->n = (f1->n * (f2->d / f1->d)) + f2->n;
		}

		this->normalize();
	}

	void normalize() {
		while (!(this->n % 2) && this->d >= 2) {
			this->n /= 2;
			this->d /= 2;
		}
	}
};

bool operator== (const Fraction &f1, const Fraction &f2)
{
	return f1.n * f2.d == f2.n * f1.d;
}

bool operator!= (const Fraction &f1, const Fraction &f2)
{
	return !(f1 == f2);
}

bool operator> (const Fraction &f1, const Fraction &f2)
{
	return f1.n * f2.d > f2.n * f1.d;
}
bool operator>= (const Fraction &f1, const Fraction &f2)
{
	return f1.n * f2.d >= f2.n * f1.d;
}
bool operator< (const Fraction &f1, const Fraction &f2)
{
	return f1.n * f2.d < f2.n * f1.d;
}
bool operator<= (const Fraction &f1, const Fraction &f2)
{
	return f1.n * f2.d <= f2.n * f1.d;
}
