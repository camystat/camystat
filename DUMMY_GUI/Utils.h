#pragma once
#include <vector>
#include <iostream>
#include "V3.h"
#include <Python.h>
#include "MainFrame.h"
#include <wx/wx.h>
#include "wx/setup.h"
#include "Utils.h"
#include <shlobj.h>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cmath>
#include "Savgol.h"
#include <Windows.h>
#include <string>

namespace fs = std::filesystem;

class Utils
{
public:
	static std::vector<double> clone_trimmed_list(const std::vector<double>& lst, size_t n, size_t x);
	static void plotVector(const std::vector<double>& values, const std::string& name, size_t min, size_t max);
	static std::vector<double> aggregate(const std::string& videoPath, const std::string& compressedPath, const std::string& heatmapPath, const std::string& coordPath, const std::string& resultPath);
	static void plot_events(const std::vector<double>& values, const std::vector<std::vector<double>>& events);
	static void write_vector_to_file(const std::vector<std::vector<double>>& data, const std::string& filename);
	static std::string TCHARToString(const TCHAR* tcharStr);

	// Function to convert std::vector<double> to PyList
	static PyObject* vector_to_pylist(const std::vector<double>& vec) {
		PyObject* pyList = PyList_New(vec.size());
		for (size_t i = 0; i < vec.size(); ++i) {
			PyList_SetItem(pyList, i, PyFloat_FromDouble(vec[i]));
		}
		return pyList;
	}

	// Function to convert std::vector<std::vector<double>> to PyList of lists
	static PyObject* vector_of_vectors_to_pylist(const std::vector<std::vector<double>>& vec) {
		PyObject* pyList = PyList_New(vec.size());
		for (size_t i = 0; i < vec.size(); ++i) {
			PyObject* subList = PyList_New(vec[i].size());
			for (size_t j = 0; j < vec[i].size(); ++j) {
				PyList_SetItem(subList, j, PyFloat_FromDouble(vec[i][j]));
			}
			PyList_SetItem(pyList, i, subList);
		}
		return pyList;
	}

	static std::string PyObjectToString(PyObject* obj) {
		if (obj == nullptr || !PyUnicode_Check(obj)) {
			{
				wxMessageDialog dialog(NULL, "Invalid PyObject or not a string", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
				dialog.ShowModal();
			}
			throw std::runtime_error("Invalid PyObject or not a string");
		}

		// Convert PyObject to a Unicode string
		PyObject* pyStr = PyUnicode_AsEncodedString(obj, "utf-8", "Error");
		if (pyStr == nullptr) {
			{
				wxMessageDialog dialog(NULL, "Failed to convert PyObject to string", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
				dialog.ShowModal();
			}
			throw std::runtime_error("Failed to convert PyObject to string");
		}

		// Convert PyBytesObject to std::string
		std::string result = PyBytes_AS_STRING(pyStr);

		// Clean up
		Py_XDECREF(pyStr);

		return result;
	}

	static std::string GetPythonErrorAsString() {
		std::string errorMessage;

		// Fetch the Python error type, value, and traceback
		PyObject* type, * value, * traceback;
		PyErr_Fetch(&type, &value, &traceback);

		// Print error type, value, and traceback to a string stream
		if (value != nullptr) {
			PyObject* valueStr = PyObject_Str(value); // Convert value to string
			if (valueStr != nullptr) {
				PyObject* bytes = PyUnicode_AsEncodedString(valueStr, "utf-8", "Error");
				if (bytes != nullptr) {
					errorMessage = PyBytes_AS_STRING(bytes);
					Py_DECREF(bytes);
				}
				Py_DECREF(valueStr);
			}
		}

		if (traceback != nullptr) {
			PyObject* tracebackStr = PyObject_Str(traceback); // Convert traceback to string
			if (tracebackStr != nullptr) {
				PyObject* bytes = PyUnicode_AsEncodedString(tracebackStr, "utf-8", "Error");
				if (bytes != nullptr) {
					errorMessage += "\nTraceback:\n";
					errorMessage += PyBytes_AS_STRING(bytes);
					Py_DECREF(bytes);
				}
				Py_DECREF(tracebackStr);
			}
		}

		// Clean up references
		Py_XDECREF(type);
		Py_XDECREF(value);
		Py_XDECREF(traceback);

		return errorMessage;
	}

	static void call_python_function(const std::string& script, const std::string& function_name,
		const std::vector<double>& values, const std::vector<std::vector<double>>& events,
		const std::string& video_name, const std::string& save_path, int fps) {
		// Initialize the Python interpreter
		Py_Initialize();

		// Add path to Python module
		//PyObject* path = PySys_GetObject("path");

		//displaying the path
		/*if (path == nullptr) {
			{
				wxMessageDialog dialog(NULL, "Failed to get sys.path", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
				dialog.ShowModal();
			}
			std::cerr << "Failed to get sys.path" << std::endl;
			Py_Finalize();
		}

		if (!PyList_Check(path)) {
			{
				wxMessageDialog dialog(NULL, "sys.path is not a list", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
				dialog.ShowModal();
			}
			std::cerr << "sys.path is not a list" << std::endl;
			Py_Finalize();
		}

		PyObject* item;
		Py_ssize_t size = PyList_Size(path);
		for (Py_ssize_t i = 0; i < size; ++i) {
			item = PyList_GetItem(path, i);
			if (item != nullptr && PyUnicode_Check(item)) {
				std::string itemStr = PyObjectToString(item);
				{
					wxMessageDialog dialog(NULL, "Path " + itemStr, wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
					dialog.ShowModal();
				}
			}
			else {
				{
					wxMessageDialog dialog(NULL, "Item at index " + std::to_string(i) + " is not a string", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
					dialog.ShowModal();
				}
			}
		}*/

		// Convert sys.path to a string (this is a list, so we need to handle it differently)
		/*PyObject* sysPathStr = PyObject_Repr(path);
		std::string pathStr = PyObjectToString(sysPathStr);
		{
			wxMessageDialog dialog(NULL, PyObjectToString(path), wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
		}
		Py_XDECREF(path);*/

		PyObject* sysPath = PySys_GetObject("path");
		if (sysPath == nullptr || !PyList_Check(sysPath)) {
			std::cerr << "Failed to get or check sys.path" << std::endl;
			Py_Finalize();
		}

		// Add the directory containing your script to sys.path
		std::filesystem::path currentPath = std::filesystem::current_path();

		// Convert the path to a string
		std::string script_directory = currentPath.string();
		{
			wxMessageDialog dialog(NULL, "The directory of has been added to the path" + script_directory, wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
		}

		PyObject* path = PyUnicode_DecodeFSDefault(script_directory.c_str());
		PyList_Append(sysPath, path);
		Py_DECREF(path);

		// Load the Python script
		PyObject* pName = PyUnicode_DecodeFSDefault(script.c_str());
		{
			wxMessageDialog dialog(NULL, "Script loaded successfully!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
		}
		PyObject* pModule = PyImport_Import(pName);
		{
			wxMessageDialog dialog(NULL, "Module loaded successfully!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
		}
		Py_XDECREF(pName);

		if (pModule != nullptr) {
			// Get the function
			PyObject* pFunc = PyObject_GetAttrString(pModule, function_name.c_str());
			{
				wxMessageDialog dialog(NULL, "Function loaded successfully!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
				dialog.ShowModal();
			}

			if (pFunc && PyCallable_Check(pFunc)) {
				{
					wxMessageDialog dialog(NULL, "Function is callable!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
					dialog.ShowModal();
				}
				// Convert C++ vectors to Python lists
				PyObject* pValues = vector_to_pylist(values);
				PyObject* pEvents = vector_of_vectors_to_pylist(events);
				PyObject* pVideoName = PyUnicode_FromString(video_name.c_str());
				PyObject* pSavePath = PyUnicode_FromString(save_path.c_str());
				PyObject* pFps = PyLong_FromLong(fps);

				// Call the Python function
				PyObject* pArgs = PyTuple_Pack(5, pValues, pEvents, pVideoName, pSavePath, pFps);
				PyObject* pValue = PyObject_CallObject(pFunc, pArgs);
				Py_XDECREF(pArgs);
				{
					wxMessageDialog dialog(NULL, "Arguments have been converted!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
					dialog.ShowModal();
				}

				if (pValue != nullptr) {
					std::cout << "Function executed successfully" << std::endl;
					{
						wxMessageDialog dialog(NULL, "Function executed successfully", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
						dialog.ShowModal();
					}
					Py_XDECREF(pValue);
				}
				else {
					{
						std::string error = GetPythonErrorAsString();
						wxMessageDialog dialog(NULL, error, wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
						dialog.ShowModal();
						PyErr_Clear(); // Clear the error
					}
					{
						wxMessageDialog dialog(NULL, "Call failed", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
						dialog.ShowModal();
					}
					std::cerr << "Call failed" << std::endl;
				}

				// Clean up
				Py_XDECREF(pFunc);
				Py_XDECREF(pModule);
				Py_XDECREF(pValues);
				Py_XDECREF(pEvents);
				Py_XDECREF(pVideoName);
				Py_XDECREF(pSavePath);
				Py_XDECREF(pFps);
				{
					wxMessageDialog dialog(NULL, "Successfully cleaned up!", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
					dialog.ShowModal();
				}
			}
			else {
				{
					std::string error = GetPythonErrorAsString();
					wxMessageDialog dialog(NULL, error, wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
					dialog.ShowModal();
					PyErr_Clear(); // Clear the error
				}
				{
					wxMessageDialog dialog(NULL, "Function not found or not callable", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
					dialog.ShowModal();
				}
				std::cerr << "Function not found or not callable" << std::endl;
			}
		}
		else {
			{
				std::string error = GetPythonErrorAsString();
				wxMessageDialog dialog(NULL, error, wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
				dialog.ShowModal();
				PyErr_Clear(); // Clear the error
			}

			{
				wxMessageDialog dialog(NULL, "ERROR: Failed to load module", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
				dialog.ShowModal();
			}
			std::cerr << "Failed to load module" << std::endl;
		}

		// Finalize the Python interpreter
		Py_Finalize();
	}

	static std::wstring s2ws(const std::string& s) {
		std::wstring ws(s.begin(), s.end());
		return ws;
	}

	// Helper function to convert std::string to std::wstring
	static std::wstring stringToWString(const std::string& str) {
		return std::wstring(str.begin(), str.end());
	}

	static void callPlotEvents(const std::string& exePath,
		const std::string& valuesPath,
		const std::string& eventsPath,
		const std::string& videoName,
		const std::string& savePath,
		int fps,
		const std::string& pathToRemove) {

		std::string command = "\"" + exePath + "\" \"" + valuesPath + "\" \"" + eventsPath + "\" \"" + videoName + "\" \"" + savePath + "\" " + std::to_string(fps) + " \"" + pathToRemove + "\"";
		std::cout << "Command: " << command << std::endl;

		// Display command using wxMessageDialog
		//wxMessageDialog dialog(NULL, command, wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
		//dialog.ShowModal();

		// Convert std::string to std::wstring for WinAPI functions
		std::wstring commandW = stringToWString(command);

		// Method: Using CreateProcess with hidden window
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		si.dwFlags |= STARTF_USESHOWWINDOW;  // Use the wShowWindow member
		si.wShowWindow = SW_HIDE;            // Set wShowWindow to hide the window

		ZeroMemory(&pi, sizeof(pi));
		// Prepare the command as a writable wide char array
		wchar_t commandWCStr[2048];
		wcscpy_s(commandWCStr, commandW.c_str());
		bool resultCreateProcess = CreateProcess(NULL,           // No module name (use command line)
			commandWCStr,   // Command line (wide char)
			NULL,           // Process handle not inheritable
			NULL,           // Thread handle not inheritable
			FALSE,          // Set handle inheritance to FALSE
			CREATE_NO_WINDOW, // Hide the window
			NULL,           // Use parent's environment block
			NULL,           // Use parent's starting directory 
			&si,            // Pointer to STARTUPINFO structure
			&pi);           // Pointer to PROCESS_INFORMATION structure

		if (resultCreateProcess) {
			// Wait for the process to finish
			WaitForSingleObject(pi.hProcess, INFINITE);
			DWORD exitCode;
			GetExitCodeProcess(pi.hProcess, &exitCode);

			// Log CreateProcess result code
			std::cout << "CreateProcess Exit Code: " << exitCode << std::endl;

			if (exitCode != 0) {
				wxMessageDialog dialog(NULL, "Plotting script finished with a non-zero exit code!.", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
				dialog.ShowModal();
			}

			// Close process and thread handles
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		else {
			// Log CreateProcess error
			DWORD error = GetLastError();
			std::cout << "CreateProcess Failed with Error Code: " << error << std::endl;

			wxMessageDialog dialog(NULL, "Failed to execute plotting script using CreateProcess.", wxMessageBoxCaptionStr, wxOK | wxCENTER | wxDIALOG_NO_PARENT);
			dialog.ShowModal();
		}

		// Save command to a file
		std::string filePath = "command.txt";
		std::ofstream outFile(filePath);
		if (outFile.is_open()) {
			outFile << command;
			outFile.close();
		}
	}

	static void writeVectorToFile(const std::string& filePath, const std::vector<double>& vec) {
		std::ofstream outFile(filePath);
		if (outFile.is_open()) {
			for (const auto& value : vec) {
				outFile << value << ",\n";
			}
			outFile.close();
		}
		else {
			std::cerr << "Failed to open file for writing: " << filePath << std::endl;
		}
	}

	static void writeVectorOfVectorsToFile(const std::string& filePath, const std::vector<std::vector<double>>& vecOfVecs) {
		std::ofstream outFile(filePath);
		if (outFile.is_open()) {
			for (const auto& vec : vecOfVecs) {
				for (size_t i = 0; i < vec.size(); ++i) {
					outFile << vec[i];
					if (i < vec.size() - 1) {
						outFile << ",";
					}
				}
				outFile << "\n";
			}
			outFile.close();
		}
		else {
			std::cerr << "Failed to open file for writing: " << filePath << std::endl;
		}
	}

	static std::vector<std::vector<double>> normalizeSecondColumnInCopy(const std::vector<std::vector<double>>& results) {
		std::vector<std::vector<double>> modifiedResults = results;

		if (results.empty()) return modifiedResults; // Return empty if no results

		// Extract the second column values from the original data
		std::vector<double> secondColumn;
		for (const auto& row : results) {
			if (row.size() >= 2) { // Ensure the row has at least two elements
				secondColumn.push_back(row[1]);
			}
		}

		if (secondColumn.empty()) return modifiedResults; // Return empty if second column is empty

		// Find the minimum and maximum values in the second column
		double minVal = *std::min_element(secondColumn.begin(), secondColumn.end());
		double maxVal = *std::max_element(secondColumn.begin(), secondColumn.end());

		if (minVal == maxVal) return modifiedResults; // Handle case where all values are the same

		// Normalize the second column values in the copy of the data
		for (auto& row : modifiedResults) {
			if (row.size() >= 2) { // Ensure the row has at least two elements
				row[1] = (row[1] - minVal) / (maxVal - minVal);
			}
		}

		return modifiedResults;
	}
};
