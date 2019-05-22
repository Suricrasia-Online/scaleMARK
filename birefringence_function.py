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
	return np.clip(nonlinearity(rgb), 0, 1)

def srgb_to_termstring(rgb):
	rgb = (rgb*255).flat
	return "\x1b[48;2;%d;%d;%dm \x1b[0m" % (int(rgb[0]), int(rgb[1]), int(rgb[2]))

#from https://scipython.com/blog/converting-a-spectrum-to-a-colour/
from scipy.constants import h, c, k, pi
def planck(lam, T):
	""" Returns the spectral radiance of a black body at temperature T.

	Returns the spectral radiance, B(lam, T), in W.sr-1.m-2 of a black body
	at temperature T (in K) at a wavelength lam (in nm), using Planck's law.

	"""

	lam_m = lam / 1.e9
	fac = h*c/lam_m/k/T
	B = 2*h*c**2/lam_m**5 / (np.exp(fac) - 1)
	return B

def birefringence_lag(lag, lam):
	return np.sin(pi*lag/lam)**2

def spectrum_from_csv(csvfile):
	data = np.genfromtxt(csvfile, delimiter=',')
	return interp_func(data[:,0], data[:,1])

def main():
	flourescent_spectrum_1 = spectrum_from_csv('flourescent_spectrum.csv')
	flourescent_spectrum_2 = spectrum_from_csv('flourescent_spectrum_2.csv')
	flourescent_spectrum_3 = spectrum_from_csv('flourescent_spectrum_3.csv')

	spectral_to_xyz = build_spectral_to_xyz()

	def spectral_to_linear_srgb(spectral):
		return xyz_to_linear_srgb(spectral_to_xyz(spectral))

	def build_birefringence_spectrum(light_source):
		spectrum = []
		maxcol = 0
		for lag in range(0, 2000, 15):
			color = spectral_to_linear_srgb(lambda x : birefringence_lag(lag, x)*light_source(x))
			maxcol = max(np.max(color), maxcol)
			spectrum.append(color)

		spectrum_string = ""
		for color in spectrum:
			color /= maxcol
			spectrum_string += srgb_to_termstring(linear_srgb_to_rgb(color))
		return spectrum_string+"\n"+spectrum_string

	blackbody_12000K = lambda x: planck(x, 12000)
	blackbody_7500K = lambda x: planck(x, 7500)
	blackbody_5000K = lambda x: planck(x, 5000)
	blackbody_2500K = lambda x: planck(x, 2500)

	print(build_birefringence_spectrum(blackbody_12000K))
	print(build_birefringence_spectrum(blackbody_7500K))
	print(build_birefringence_spectrum(blackbody_5000K))
	print(build_birefringence_spectrum(blackbody_2500K))
	print(build_birefringence_spectrum(flourescent_spectrum_3))
	print(build_birefringence_spectrum(flourescent_spectrum_1))
	print(build_birefringence_spectrum(flourescent_spectrum_2))



if __name__ == "__main__":
	main();
