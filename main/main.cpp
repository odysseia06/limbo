#include <iostream>
#include "ErrorLog.h"

int main() {
	LErrorLogManager* Log = LErrorLogManager::GetErrorManager();
	Log->create("testlog.txt");
	try {
		THROW_EXCEPTION(1, "This is a test error");
	}
	catch (LException& e) {
		std::cout << e.what() << std::endl;
		Log->m_LogBuffer << "*****ERROR******\n";
		Log->flush();
		Log->logException(e);
		Log->m_LogBuffer << "***************\n";
		Log->flush();
	}
	Log->close();
	return 0;
}