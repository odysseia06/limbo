#include "ErrorLog.h"

LException::LException(int ErrorNumber, std::string ErrorDesc, std::string SrcFileName, int LineNumber) {
	m_ErrorNumber = ErrorNumber;
	m_ErrorDesc = ErrorDesc;
	m_SrcFileName = SrcFileName;
	m_LineNumber = LineNumber;

	std::stringstream ErrStr;

	ErrStr << "Error Num: " << m_ErrorNumber << "\nError Desc: " << m_ErrorDesc << "\nSrc File: " << m_SrcFileName
		<< "\nLine Number: " << m_LineNumber << "\n";
	m_ErrText = ErrStr.str();
}

const char* LException::what() const {
	return m_ErrText.c_str();
}

LErrorLogManager::LErrorLogManager() {

}

void LErrorLogManager::create(std::string Filename) {
	m_LogFile.open(Filename.c_str());
}

void LErrorLogManager::flush() {
	m_LogFile << m_LogBuffer.str();
	m_LogFile.flush();
	m_LogBuffer.str("");
}

void LErrorLogManager::close() {
	m_LogFile.close();
}

void LErrorLogManager::logException(LException e) {
	m_LogBuffer << getTimeString() << "\n" << e.what();
	flush();
}

std::string LErrorLogManager::getTimeString()
{
	// Gets current time as string in the form h:m:s
	// Might change this later
	std::stringstream TimeStr;
	struct tm currentTime;
	time_t ctTime; time(&ctTime);
	localtime_s(&currentTime, &ctTime);

	TimeStr << std::setw(2) << std::setfill('0') << currentTime.tm_hour << ":";
	TimeStr << std::setw(2) << std::setfill('0') << currentTime.tm_min << ":";
	TimeStr << std::setw(2) << std::setfill('0') << currentTime.tm_sec;

	return TimeStr.str();
}

LErrorLogManager LErrorLogManager::m_ErrorManager;

LErrorLogManager* LErrorLogManager::GetErrorManager() {
	return &m_ErrorManager;
}