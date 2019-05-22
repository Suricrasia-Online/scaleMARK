#!/usr/bin/env python3

import numpy as np
import scipy.integrate as integrate

def interp_func(vx, vy):
	return lambda x : np.interp(x, vx, vy, 0, 0)

def spectral_dot(func1, func2):
	# spectral integration ranges
	xmin = 380
	xmax = 780
	return integrate.fixed_quad(lambda x : func1(x) * func2(x), xmin, xmax, n=1000)[0]

def build_spectral_to_xyz():
	color_matching_data = np.genfromtxt('lin2012xyz2e_1_7sf.csv', delimiter=',')
	color_matching_x = interp_func(color_matching_data[:,0], color_matching_data[:,1])
	color_matching_y = interp_func(color_matching_data[:,0], color_matching_data[:,2])
	color_matching_z = interp_func(color_matching_data[:,0], color_matching_data[:,3])

	return lambda spectral: np.array([spectral_dot(spectral, color_matching_x), spectral_dot(spectral, color_matching_y), spectral_dot(spectral, color_matching_z)])

def xyz_to_linear_srgb(xyz):
	xyztosrgb = np.matrix([
		[ 3.2404542, -1.5371385, -0.4985314],
		[-0.9692660,  1.8760108,  0.0415560],
		[ 0.0556434, -0.2040259,  1.0572252]])
	return np.clip(xyztosrgb.dot(xyz), 0, np.inf)

def normalize_rgb(rgb):
	if not np.all(rgb == 0):
		rgb /= np.max(rgb) #normalize (you're not normally supposed to do this...)
	return rgb

def linear_srgb_to_rgb(rgb):
	nonlinearity = np.vectorize(lambda x: 12.92*x if x < 0.0031308 else 1.055*(x**(1.0/2.4))-0.055)
	return nonlinearity(rgb)

def srgb_to_termstring(rgb):
	rgb = (rgb*255).flat
	return "\x1b[48;2;%d;%d;%dm    \x1b[0m" % (int(rgb[0]), int(rgb[1]), int(rgb[2]))

#from https://scipython.com/blog/converting-a-spectrum-to-a-colour/
from scipy.constants import h, c, k
def planck(lam, T):
	""" Returns the spectral radiance of a black body at temperature T.

	Returns the spectral radiance, B(lam, T), in W.sr-1.m-2 of a black body
	at temperature T (in K) at a wavelength lam (in nm), using Planck's law.

	"""

	lam_m = lam / 1.e9
	fac = h*c/lam_m/k/T
	B = 2*h*c**2/lam_m**5 / (np.exp(fac) - 1)
	return B

def spectrum_from_csv(csvfile):
	data = np.genfromtxt(csvfile, delimiter=',')
	return interp_func(data[:,0], data[:,1])

def main():
	flourescent_spectrum_1 = spectrum_from_csv('flourescent_spectrum.csv')
	flourescent_spectrum_2 = spectrum_from_csv('flourescent_spectrum_2.csv')
	flourescent_spectrum_3 = spectrum_from_csv('flourescent_spectrum_3.csv')

	spectral_to_xyz = build_spectral_to_xyz()

	spectral_to_linear_srgb = lambda spectral: normalize_rgb(xyz_to_linear_srgb(spectral_to_xyz(spectral)))

	for wavelength in range(500, 12000+1, 500):
		color1 = srgb_to_termstring(spectral_to_linear_srgb(lambda x : planck(x, wavelength)))
		color2 = srgb_to_termstring(linear_srgb_to_rgb(spectral_to_linear_srgb(lambda x : planck(x, wavelength))))
		print(color1, color2, "%dK" % wavelength)


	for flourescent_spectrum in [flourescent_spectrum_1, flourescent_spectrum_2, flourescent_spectrum_3]:
		print(srgb_to_termstring(spectral_to_linear_srgb(flourescent_spectrum)), srgb_to_termstring(linear_srgb_to_rgb(spectral_to_linear_srgb(flourescent_spectrum))))

if __name__ == "__main__":
	main();
