from conans import ConanFile


class PuggConan(ConanFile):
	name = "pugg"
	version = "1.0.0"
	license = "https://www.apache.org/licenses/LICENSE-2.0"
	url = "https://github.com/tuncb/pugg"
	description = "A Simple C++ Framework to load classes from dll files in an OO way."

	def package(self):
		self.copy("*", dst="pugg", src="./projects/pugg/include/pugg")

	def package_info(self):
		self.cpp_info.includedirs = ["."]
